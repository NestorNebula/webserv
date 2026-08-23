/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 11:01:41 by kdonlon          ###   ########.fr       */
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
	try 
	{
		if (this->conn)
		{
			WSLOG(LVL_DBG, TGT_CGI, " (~) conn_fd ", this->conn->get_fd());
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


bool	CgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CGI_TIMEOUT) > now)
		return (false);
	

	this->lact = now;
	
	WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : pipe ", this->get_fd());
	if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : conn ", conn->get_fd());
	}
	if (this->rsrc && this->conn)
	{
		if (this == this->rsrc->ip)
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : pipe (ip)");
		}
		else if (this == this->rsrc->op)
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : pipe (op)");
		}

try {
		Session &sess = conn->sess;
		Request &req  = sess.getRequest();
		if (req.hasHeaders())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : has headers");
		}
		if (req.hasBody())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : has body");
		}
		if (req.isComplete())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : complete");
		}
}
catch(const std::exception& e)
{
	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO\n", e.what());
}
		rsrc->set_done(RSRC_DONE_ERR);
		this->rsrc->set_err(504); 
	}
	else if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : conn ", conn->get_fd());
		this->conn->set_err(504);
	}
	else
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : ???? ");
	}

	
	return (false);
#if 0
	if (this->rsrc && this->conn)
	{
		// MOSTLY (ip) .. but .. 
		if (this == this->rsrc->ip)
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "pipe: TIMEO (ip)"); // biguadio.php can get blocky
		}
		else if (this == this->rsrc->op)
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "pipe: TIMEO (op)");
		}
			
#if 1 // Exceptions suck
		Session &sess = conn->sess;
		Request &req  = sess.getRequest();
		if (req.hasHeaders())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : has headers");
		}
		if (req.hasBody())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : has body");
		}
		if (req.isComplete())
		{
			WSLOG(LVL_DBG, TGT_CGI_SEND, "req : complete"); // , ++to); // 1500 (!)
			if (this == this->rsrc->ip)
			{
// timeout on IP is allowed .. 
// though .. it SHOULD have been shut down
// when the request was fully sent to it
				// expected .. if delivering large file 
				this->lact = now;
				// this->mod_evt(-EPOLLOUT);
				// this->rsrc->rem(this);
				return (false);
			}
		}
#endif
		this->rsrc->set_err(504); // Gateway Timeout
	}
	else if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : conn");
		this->conn->set_err(504); // Gateway Timeout
	}
	else
	{
		WSLOG(LVL_DBG, TGT_CGI_SEND, "TIMEO : ????");
		// (conn) does not exist !
		// this->conn->set_err(504); // Gateway Timeout
	}
	return (true);
#endif
}

// The server is in no way obligated to send end-of-file 
// after the script reads CONTENT_LENGTH bytes. 
ssize_t	CgiPipe::pollout(void)
{
	WSLOG(LVL_DBG, TGT_CGI_SEND, "send:  POLLOUT");
	
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

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
		this->rsrc->rem(this);
		rsrc->set_done(RSRC_DONE_IP);
		return (-1);
	default:
		break;
	}

	err = this->send(rsrc->body);
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_CGI_SEND, "send");
		return (this->rsrc->set_err(500)); // Internal Server Error
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
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	WSLOG(LVL_DBG, TGT_CGI_RECV, "recv:  POLLIN");
	WSLOG(LVL_DBG, TGT_CGI_RECV, "conn: ", this->conn->get_fd());
	
	ssize_t	err = 0;
	
	err = this->recv();
	WSLOG(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_CGI_RECV, "recv: err");
		return (this->rsrc->set_err(500)); // Internal Server Error
	}
	if (err == 0)
	{
		WSLOG(LVL_DBG, TGT_CGI_RECV, "recv:  ZERO");
		rsrc->set_done(RSRC_DONE_OP);
#if RES_CGI_WAIT_COMPLETE
		conn->mod_evt(EPOLLOUT);
#endif
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
#if !RES_CGI_WAIT_COMPLETE
		conn->mod_evt(EPOLLOUT);
#endif
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
	this->conn = NULL;
	this->rsrc = NULL;
}





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


