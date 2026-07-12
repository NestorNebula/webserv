/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/12 21:59:40 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "Connection.hpp"
#include "Server.hpp"

cgi_pipes::cgi_pipes (void)
{
	p1[0] = -1;
	p1[1] = -1;
	p2[0] = -1;
	p2[1] = -1;
}

cgi_pipes::~cgi_pipes()
{
	this->shutdown();
}

int	cgi_pipes::init(void)
{
	int	err;
	
	err = pipe(p1);
	if (err < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	err = pipe(p2);
	if (err < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	return (0);
}

int	cgi_pipes::dup_io(void)
{
	int	err;

	if (p1[0] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	err = dup2(p1[0], STDIN_FILENO);
	if (err < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}

	if (p2[1] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	err = dup2(p2[1], STDOUT_FILENO);
	if (err < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	
#if 0
	int dnfd = open("/dev/null", O_WRONLY);
	err = dup2(dnfd, STDERR_FILENO);
	if (err < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	close(dnfd);
#endif
	return (err);		
}

void	cgi_pipes::shutdown(void)
{
	if (p1[0] != -1)
	{
		close(p1[0]);
		p1[0] = -1;
	}
	if (p1[1] != -1)
	{
		close(p1[1]);
		p1[1] = -1;
	}
	if (p2[0] != -1)
	{
		close(p2[0]);
		p2[0] = -1;
	}
	if (p2[1] != -1)
	{
		close(p2[1]);
		p2[1] = -1;
	}
}




CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn)
{
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
	if (this->conn)
		this->conn->rem_cgi(this);
}

ssize_t	CgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
		
	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv: err");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: zero");
		return (-1);
	}
		
	this->conn->ostr.append(this->ibuf, err);
	this->conn->mod_evt(EPOLLOUT);
	// this mod pollin -- but !!! that is a SEPARATE CgiPipe
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", conn->ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", this->conn->ostr);
	
	return (err);
}

ssize_t	CgiPipe::pollout(void)
{
	if (this->conn == NULL)
		return (-1);
		
	ssize_t	err;
    
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", this->conn->istr.size());

	// cgi : needs to have received CONTENT_LENGTH .. 
	// so it knows something is coming 
	// otherwise we need to CLOSE its INPUT
	// conn::state (read_data) 

	// rsrc.data_ip
	if (this->conn->istr.size())
	{
		// WsLog::_(LVL_DBG, TGT_CGI_DATA, "send\n", this->conn->istr);
		err = this->send(this->conn->istr);
		if (err < 0)
		{
			WsLog::_(LVL_ERR, TGT_CGI_SEND, "send");
			return (err);
		}
		if (err == 0)
		{
			WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: zero");
			return (0);
		}
		// in principle
		// BUT : we want to kick in TOGGLE SOONER
		// BUT : we won't have the error
		// until we've started to receive
		if (this->conn->state < RSRC_HAS_RESP)
			this->conn->state = RSRC_HAS_RESP;
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
		return (0); // keep going
	}
	// hm : Conn should decide when it is done writing
	// BUT : this may close on cgi fail (?)
	this->mod_evt(0);
	this->conn->mod_evt(EPOLLOUT);
	return (0);

}


// cgi::hup .. set conn state DONE 
// epoll::state (read_data)
	// has read data from its (fd) 
	// that is avaiable for processing (in its ibuf)
int		CgiPipe::hup(void)
{
	if (this->conn == NULL)
		return (-1);
	// if POLLIN
		// set_state() could better protect/compare 
	// check (pid) here (?)
	// might be (ip) .. which hangs up (when stdin is closed)
	
	// YEAH -- conn->cgi_hup() : TEST 
	// NOT NECESSARILY
	// CONN_SENT_RESP may not yet have been triggered
	// on delete .. 
	// RSRC_COMPLETE -- may have error
	// check (pid) here (?)
	this->conn->state = RSRC_BODY_DONE;
	this->conn->mod_evt(EPOLLOUT);
	return (-1);
}