/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/11 12:09:33 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"
#include "Connection.hpp"
#include "Server.hpp"


static	void fd_close(int *fd)
{
	if (*fd == -1)
		return;
	close(*fd);
	*fd = -1;
}

cgi_pipes::cgi_pipes (void)
{
	p1[0] = -1;
	p1[1] = -1;
	p2[0] = -1;
	p2[1] = -1;
	dnfd  = -1;
}

cgi_pipes::~cgi_pipes()
{
	this->shutdown();
}

int	cgi_pipes::init(void)
{
	if (pipe(p1) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	if (pipe(p2) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	return (0);
}

int	cgi_pipes::dup_io(void)
{
	if (p1[0] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p1[0], STDIN_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stdin)"));
	}
	if (p2[1] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p2[1], STDOUT_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stdout)"));
	}
	return (0);		
}

int	cgi_pipes::dup_err(void)
{
	dnfd = open("/dev/null", O_WRONLY);
	if (dnfd < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "open (/dev/null)"));
	}
	if (dup2(dnfd, STDERR_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stderr)"));
	}
	fd_close(&dnfd);
	return (0);
}

void	cgi_pipes::shutdown(void)
{
	fd_close(p1);
	fd_close(p1 + 1);
	fd_close(p2);
	fd_close(p2 + 1);
	fd_close(&dnfd);
}




CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourcePiped * _rsrc) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn),
	rsrc(_rsrc)
{
	sock_non_block(this->fd);
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, " (~) Cgi");
	if (this->conn)
		this->conn->cgi_rem(this);
	// if (this->rsrc)
	// 	this->rsrc->rem(this);
}

bool	CgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CGI_TIMEOUT) < now)
	{
		if (this->rsrc)
			this->rsrc->set_err(504);
		else if (this->conn)
			this->conn->set_err(504);
		return (true);
	}
	return (false);
}

ssize_t	CgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv: err");
		this->rsrc->set_err(501); // CGI_ERR : read failed
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv:  ZERO");
		return (-1);
	}
	
	switch (this->rsrc->recv_data(this->ibuf, err))
	{
	case RSRC_RESP_INIT:
		break;
	case RSRC_RESP_ERR:
		conn->set_err(rsrc->error);
		break;
	case RSRC_RESP_HEAD:
		break;
	case RSRC_RESP_BODY:
	default:
		conn->mod_evt(EPOLLOUT);
		break;
	}
	return (err);
}

// The server is in no way obligated to send end-of-file 
// after the script reads CONTENT_LENGTH bytes. 
ssize_t	CgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	err = this->conn->req_body_status();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body     : complete");
		return (-1);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body     : waiting");
		this->mod_evt(0);
		return (0);
	}
	
	std::string & body = this->conn->sess.req.get_body();
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", body.size());
	err = this->send(body);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_SEND, "send");
		this->rsrc->set_err(502); // CGI_ERR : write failed
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "send:  ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
	return (0);
}

int		CgiPipe::rdhup(void)
{
	return (-1);
}

int		CgiPipe::hup(void)
{
	return (-1);
}

void	CgiPipe::rsrc_closed(void)
{ 
	this->conn = NULL;
	this->rsrc = NULL;
}


