/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/17 11:41:59 by kdonlon          ###   ########.fr       */
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
	int dnfd = open("/dev/null", O_WRONLY);
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
	close(dnfd);
	return (0);
}

static	void fd_close(int *fd)
{
	if (*fd == -1)
		return;
	close(*fd);
	*fd = -1;
}

void	cgi_pipes::shutdown(void)
{
	fd_close(p1);
	fd_close(p1 + 1);
	fd_close(p2);
	fd_close(p2 + 1);
}




CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn)
{
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
	if (this->conn) // RSRC
		this->conn->cgi.rem(this);
}

ssize_t	CgiPipe::pollin(void)
{
	// sess ..
	if (this->conn == NULL)
		return (-1);
		
	// sess->active() -- state -- alive, no error
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
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: ZERO");
		return (-1);
	}
	// sess.push_op_data()
	// rsrc.add_data()
	if (this->conn->cgi_out(this->ibuf, err) < 0)
		return (-1);
	
	return (err);
}

ssize_t	CgiPipe::pollout(void)
{
	if (this->conn == NULL)
		return (-1);
	// sess->active()
	// sess->has_input_data() [ POST ]
	if (this->conn->cgi.status(WNOHANG) >= 0) // RSRC done .. error 
		return (-1);
		
	ssize_t	err;
    
	// rsrc.has_body_to_send_to_cgi
	// sess.has_input()
	err = this->conn->cgi_inp(); // part of cgi_status
	if (err < 0)
	{
		// nothing more to send to CGI (stdin)
		// we may close down
		return (-1);
	}
	if (err == 0)
	{
		// WsLog::_(LVL_ERR, TGT_CGI_SEND, "send: no input");
		this->mod_evt(0);
		return (0);
	}

	// rsrc.has_input
	std::string & body = this->conn->sess.req.get_body();
	
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", body.size());

	// WsLog::_(LVL_DBG, TGT_CGI_DATA, "send\n", body);
	err = this->send(body);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
	return (0);
}

int		CgiPipe::hup(void)
{
	if (this->conn == NULL)
		return (-1);
	return (-1);
}