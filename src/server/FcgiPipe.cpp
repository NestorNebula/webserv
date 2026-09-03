/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiPipe.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 21:17:36 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FcgiPipe.hpp"
#include "ResourceFcgi.hpp"

#include <string>

FcgiPipe::FcgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourceFcgi * _rsrc) : 
	EpollClient(_ep, EPC_FCGI, _fd), 
	conn(_conn),
	rsrc(_rsrc)
{
	sock_non_block(this->fd);
}
	
FcgiPipe::~FcgiPipe()
{
	WSLOG(LVL_DBG, TGT_FCGI, " (~) Fcgi ", this->fd);
	try 
	{
		if (this->conn)
		{
			WSLOG(LVL_DBG, TGT_FCGI, "     conn_fd ", this->conn->get_fd());
			this->conn->cgi_rem(this);
		}
		if (this->rsrc)
			this->rsrc->rem(this);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_FCGI, " (~) Fcgi\n", e.what());
	}
}

bool	FcgiPipe::timeo(WsTime & now)
{
	if (this->lact.not_set())
		return (false);
	if (this->lact.after(now))
		return (false);
	if ((this->lact + CGI_TIMEOUT).after(now))
		return (false);
		
	this->lact = now;

	this->mod_evt(-EPOLLOUT);
	
	WSLOG(LVL_DBG, TGT_FCGI | TGT_TIMEO, "TIMEO : fcgi ", this->get_fd());
	if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_FCGI | TGT_TIMEO, "TIMEO : conn ", conn->get_fd());
	}
	
	if (this->rsrc)
	{
		if (rsrc->done == RSRC_DONE_IO)
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_FCGI | TGT_TIMEO, "TIMEO : done");
			return (false);
		}
		
		rsrc->set_done(RSRC_DONE_ERR);
		this->rsrc->set_err(504);  // CGI_ERR : gateway timeout
	}
	else if (this->conn)
	{
		WSLOG(LVL_DBG, TGT_FCGI | TGT_TIMEO, "TIMEO : conn ", conn->get_fd());
		this->conn->set_err(504); // CGI_ERR : gateway timeout
	}
	else
	{
		WSLOG(LVL_DBG, TGT_FCGI | TGT_TIMEO, "TIMEO : ???? ");
		this->mod_evt(EPOLLOUT);
	}
	return (false);
}

int		FcgiPipe::init(CgiEnv * cgienv)
{
	int err;

	err = fcgi.req_init(cgienv);
	if (err < 0)
	{
		return (err);
	}
	return (err);
}

ssize_t	FcgiPipe::pollout(void)
{
	WSLOG(LVL_DBG, TGT_FCGI, "send:  POLLOUT");

	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
		
	WSLOG(LVL_DBG, TGT_FCGI, "send");
		
	switch(rsrc->get_req_body())
	{
	case REQ_WAIT_HEAD:
		WSLOG(LVL_DBG, TGT_FCGI, "head     : waiting");
		this->mod_evt(0);
		return (0);
	case REQ_WAIT_BODY:
		WSLOG(LVL_DBG, TGT_FCGI, "body     : waiting");
		this->mod_evt(-EPOLLOUT);
		return (0);
	case REQ_COMPLETE: 
		// WSLOG(LVL_DBG, TGT_FCGI, "req      : complete");
		// may still need to send END_STDIN
	default:
		break;
	}
	
	if ((rsrc->body.size() == 0) && (fcgi.req.size() == 0))
	{
		rsrc->set_done(RSRC_DONE_IP);
		this->mod_evt(-EPOLLOUT);
		this->mod_evt(EPOLLIN);
	}
	
	// WSLOG(LVL_DBG, TGT_FCGI, "body: ", rsrc->body.size());
	// WSLOG(LVL_DBG, TGT_FCGI, "req : ", fcgi.req.size());
	fcgi.req_body(rsrc->body);

	
	err = this->send(fcgi.req);
	WSLOG(LVL_DBG, TGT_FCGI, "req : ", fcgi.req.size());
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_FCGI, "send");
		return (this->rsrc->set_err(611)); // CGI_ERR // Internal Server Error
	}
	if (err == 0)
	{
		WSLOG(LVL_DBG, TGT_FCGI, "send:  ZERO");
		rsrc->set_done(RSRC_DONE_IP);
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	WSLOG(LVL_DBG, TGT_FCGI, "sent: ", err);
	WSLOG(LVL_DBG, TGT_FCGI, "left: ", fcgi.req.size());
	return (0);
}

ssize_t	FcgiPipe::pollin(void)
{
	ssize_t	err = 0;

	WSLOG(LVL_DBG, TGT_FCGI, "recv:  POLLIN");	

	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	WSLOG(LVL_DBG, TGT_FCGI, "recv");
	err = this->recv();
	WSLOG(LVL_DBG, TGT_FCGI, "recv: ", err);

	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_FCGI, "recv: err");
		return (this->rsrc->set_err(501)); // CGI_ERR : read failed
	}
	if (err == 0)
	{
		WSLOG(LVL_DBG, TGT_FCGI, "recv:  ZERO");
		// WSLOG(LVL_DBG, TGT_FCGI, "req : ", this->fcgi.req.size());
		// WSLOG(LVL_DBG, TGT_FCGI, "rsp : ", this->fcgi.rsp.size());
		rsrc->set_done(RSRC_DONE_OP);
		return (0);
	}
	
	if (fcgi.rsp_recv(this->ibuf, err) < 0)
	{
		WSLOG(LVL_ERR, TGT_FCGI_PARSE, "parse: failed");
		return (-1);
	}
		
	switch (this->rsrc->recv_data((char*) fcgi.rsp.c_str(), fcgi.rsp.size()))
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
	
	fcgi.rsp.clear();
	return (err);
}

int		FcgiPipe::rdhup(void)
{
	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_FCGI, "RDHUP");
	
	if (this->rsrc == NULL)
		return (-1);
	
// this is where we betray our complete lack of mastery of what is going on here.
	rsrc->set_done(RSRC_DONE_IP);
	WSLOG(LVL_DBG, TGT_FCGI, "rdhup: done ", rsrc->done);

	if (this->fcgi.req.size())
	{
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_FCGI, "rdhup: req.size() ", this->fcgi.req.size());
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_FCGI, "rdhup: should never get here!");
		return (0);
	}
	
	if (this->conn == NULL)
		return (-1);
	this->conn->mod_evt(EPOLLOUT);

	if (rsrc->set_done(0) == -1)
		return (-1);

	if (this->rsrc->resp_data()) 
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_FCGI, "rdhup: resp.size() ", this->rsrc->resp.size());
		// WSLOG(LVL_DBG, TGT_FCGI, "rdhup: resp\n", this->rsrc->resp);

		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_FCGI, "rdhup: error ", this->rsrc->error);
		return (0);
	}
	WSCOL(WSL_RED);
	WSLOG(LVL_DBG, TGT_FCGI, "rdhup: should never get here!");
	return (0);
}

int		FcgiPipe::hup(void)
{
	return (-1);
}

void	FcgiPipe::rsrc_closed(void)
{ 
	// mod_evt (?)
	this->conn = NULL;
	this->rsrc = NULL;
}



