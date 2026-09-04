/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/04 16:37:29 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "ResourcePiped.hpp"
#include "Connection.hpp"
#include "Server.hpp"
#include "Request.hpp"

CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourcePiped * _rsrc) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn),
	rsrc(_rsrc)
{
	sock_non_block(this->fd);
}

CgiPipe::~CgiPipe()
{
	WSLOG(LVL_DBG, TGT_CGI, " (~) CgiPipe ", this->fd);
	fd_close(&this->fd);
	try 
	{
		if (this->conn)
		{
			WSLOG(LVL_DBG, TGT_CGI, "     conn_fd ", this->conn->get_fd());
			this->conn->cgi_rem(this);
		}
		if (this->rsrc)
			this->rsrc->rem(this);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CGI, " (~) CgiPipe\n", e.what());
	}
}


bool	CgiPipe::timeo(WsTime & now)
{
	if (this->lact.not_set())
		return (false);
	if (this->lact.after(now))
		return (false);
	if ((this->lact + CGI_TIMEOUT).after(now))
		return (false);

	this->lact = now;
	
	WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : pipe ", this->get_fd());
	if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : conn ", conn->get_fd());
	}

	if (this->rsrc)
	{
		WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : rsrc ", conn->get_fd());
		if (rsrc->done == RSRC_DONE_IO)
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : done");
			return (false);
		}
		
		rsrc->set_done(RSRC_DONE_ERR);
		this->rsrc->set_err(504);  // CGI_ERR : gateway timeout
	}
	else if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : conn ", conn->get_fd());
		this->conn->set_err(504); // CGI_ERR : gateway timeout
	}
	else
	{
		WSLOG(LVL_DBG, TGT_CGI | TGT_TIMEO, "TIMEO : ???? ");
		this->mod_evt(EPOLLOUT);
	}
	return (false);
}

ssize_t	CgiPipe::pollout(void)
{
	WSLOG(LVL_DBG, TGT_CGI_SEND, "send:  POLLOUT");
	
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	WSLOG(LVL_DBG, TGT_CGI_SEND, "send");

	switch(rsrc->get_req_body())
	{
	case REQ_WAIT_HEAD:
		WSLOG(LVL_DBG, TGT_CGI_SEND, "head     : waiting");
		this->mod_evt(0);
		return (0);
	case REQ_WAIT_BODY:
		WSLOG(LVL_DBG, TGT_CGI_SEND, "body     : waiting");
		this->mod_evt(-EPOLLOUT);
		return (0);
	case REQ_COMPLETE:
		WSLOG(LVL_DBG, TGT_CGI_SEND, "body     : complete");
		this->rsrc->rem(this);
		rsrc->set_done(RSRC_DONE_IP);
		return (-1);
	default:
		break;
	}

	// WSLOG(LVL_DBG, TGT_CGI_SEND, "body:\n", rsrc->body);
	err = this->send(rsrc->body);
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_CGI_SEND, "send");
		return (this->rsrc->set_err(500)); // #kd (611)
	}
	if (err == 0)
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "send:  ZERO");
		rsrc->set_done(RSRC_DONE_IP);
		return (-1);
	}
	WSLOG(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
	return (0);
}

ssize_t	CgiPipe::pollin(void)
{
	ssize_t	err = 0;
	
	WSLOG(LVL_DBG, TGT_CGI_RECV, "recv:  POLLIN");
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	WSLOG(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	WSLOG(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_CGI_RECV, "recv: err");
		return (this->rsrc->set_err(500)); // #kd (611)
	}
	if (err == 0)
	{
		WSCOL(WSL_CYAN);
		WSLOG(LVL_TMP, TGT_CGI_RECV, "recv:  ZERO");
		rsrc->set_done(RSRC_DONE_OP);
		return (-1);
	}
	// this->ibuf[err] = '\0';
	// WSLOG(LVL_DBG, TGT_CGI_SEND, "recv:\n", std::string(ibuf));
	
	switch (this->rsrc->recv_data(this->ibuf, err))
	{
	case RSRC_RESP_INIT:
		break;
	case RSRC_RESP_ERR:
		this->mod_evt(-EPOLLIN);
		break;
	case RSRC_RESP_HEAD:
		break;
	case RSRC_RESP_BODY:
	default:
		break;
	}
	return (err);
}

int		CgiPipe::rdhup(void)
{
	if (this->rsrc)
		rsrc->set_done(RSRC_DONE_IP);
	return (-1);
}

int		CgiPipe::hup(void)
{
	if (this->rsrc)
		rsrc->set_done(RSRC_DONE_IP | RSRC_DONE_OP);
	return (-1);
}

void	CgiPipe::rsrc_closed(void)
{ 
	// mod_evt (?)
	this->conn = NULL;
	this->rsrc = NULL;
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
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "pipe()"));
	}
	if (pipe(p2) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "pipe()"));
	}
	return (0);
}

int	cgi_pipes::dup_io(void)
{
	if (p1[0] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p1[0], STDIN_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "dup2 (stdin)"));
	}
	if (p2[1] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p2[1], STDOUT_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "dup2 (stdout)"));
	}
	return (0);		
}

int	cgi_pipes::dup_err(void)
{
	dnfd = open("/dev/null", O_WRONLY);
	if (dnfd < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "open (/dev/null)"));
	}
	if (dup2(dnfd, STDERR_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_SYSERR, TGT_CGI, "dup2 (stderr)"));
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
