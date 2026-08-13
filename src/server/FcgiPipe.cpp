/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiPipe.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 11:49:44 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FcgiPipe.hpp"
#include "ResourceCgi.hpp"

#include <string>

FcgiPipe::FcgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourceFcgi * _rsrc) : 
	EpollClient(_ep, EPC_FCGI, _fd), 
	conn(_conn),
	rsrc(_rsrc),
	have_body(0)
{
	sock_non_block(this->fd);
}
	
FcgiPipe::~FcgiPipe()
{
	WsLog::_(LVL_DBG, TGT_FCGI, " (~) Fcgi");
	if (this->conn)
		this->conn->cgi_rem(this);
	// if (this->rsrc)
	// 	this->rsrc->rem(this);
}

bool	FcgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CGI_TIMEOUT) < now)
	{
		if (this->rsrc)
			this->rsrc->set_err(504); // CGI_ERR
		else if (this->conn)
			this->conn->set_err(504); // CGI_ERR
		return (true);
	}
	return (false);
}

int		FcgiPipe::init(CgiEnv * cgienv)
{
	int err;

	err = fcgi.req_init(cgienv);
	if (err < 0)
		return (err);
	return (err);
}

ssize_t	FcgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_FCGI, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_FCGI, "recv: ", err);

	// hm : data returned from CGI .. BEFORE "upload" is complete ... 
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "recv: err");
		this->rsrc->set_err(501); // CGI_ERR : read failed
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_FCGI, "recv:  ZERO");
		WsLog::_(LVL_DBG, TGT_FCGI, "req : ", this->fcgi.req.size());
		WsLog::_(LVL_DBG, TGT_FCGI, "body: ", this->conn->req_body_status());

		if ((this->fcgi.req.size() > 0) || (this->conn->req_body_status() >= 0))
		{
			// this->mod_evt(-EPOLLIN);
			return (0);
		}
		return (0);
	}
	
	if (fcgi.rsp_recv(this->ibuf, err) < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "parse failed");
		return (-1);
	}
		

	switch (this->rsrc->recv_data((char*) fcgi.rsp.c_str(), fcgi.rsp.size()))
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
	fcgi.rsp.clear();
	return (err);
}

// The server is in no way obligated to send end-of-file 
// after the script reads CONTENT_LENGTH bytes. 

// static int body_push = 0;

ssize_t	FcgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
	

	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT: ", fcgi.req.size());
	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT\n", fcgi.req);
// WEBSERV : SESSION
	if (this->conn->req_body_status() > 0)
	{
		std::string & body = this->conn->sess.req.get_body();
		
		WsLog::_(LVL_DBG, TGT_FCGI, "body: ", body.size());

		// body_push += body.size();
		if (body.size() == 0)
		{
			WsLog::color(WSL_RED);

			WsLog::_(LVL_DBG, TGT_FCGI, "body: ZERO");
		}

		fcgi.req_body((char*) body.c_str(), body.size());
		body.clear(); 
	}

#if 0 // HAVE_BODY
	if (!have_body && this->fcgi.req.size() == 0)
	{
		err = this->conn->req_body_status();
		if (err < 0)
		{
			WsLog::_(LVL_DBG, TGT_FCGI, "body     : complete");
			fcgi.req_body(NULL, 0);
			this->mod_evt(-EPOLLOUT);
			have_body = 1;
		}
		else if (err == 0)
		{
			// Continue -- should FAIL
			WsLog::_(LVL_DBG, TGT_FCGI, "body     : waiting");
			this->mod_evt(0);
			return (0);
		}
		else
		{
// WEBSERV : SESSION
			std::string & body = this->conn->sess.req.get_body();
			
			WsLog::_(LVL_DBG, TGT_FCGI, "send: ", body.size());

			// body_push += body.size();
			fcgi.req_body((char*) body.c_str(), body.size());
			body.clear();
			// WsLog::_(LVL_DBG, TGT_FCGI, "body\n", fcgi.req_body);
		}
	}
#endif
	
// WsLog::_(LVL_DBG, TGT_FCGI, "body:  pushed ", body_push);
	err = this->send(fcgi.req);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_FCGI, "send:  ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_FCGI, "sent: ", err);
	WsLog::_(LVL_DBG, TGT_FCGI, "left: ", fcgi.req.size());

	// if (fcgi.req.size() == 0)
	this->mod_evt(EPOLLIN);
	// if (have_body)
	// 	return (-1); // did we need to close here (?)

// are we done ?

	return (0);
}

int		FcgiPipe::rdhup(void)
{
	// nothing more to "send back"
	// but .. still may be receiving an upload
	// this->mod_evt(-EPOLLIN); // BAD IDEA
	// this->mod_evt(-EPOLLOUT);
	WsLog::_(LVL_DBG, TGT_FCGI, "RDHUP");
	
// THE QUESTION : when to die 
	// return (0); // UGLY 

	if (this->fcgi.req.size())
		return (0);

// we may (rdhup) 
// but can't CLOSE until BOTH SIDES ARE DONE 
// STATE CHECK
// find TEST CASES
	// if (have_body == 0)
	// 	return (0);
	// still need to send BODY_DONE 
	if (this->rsrc->ostr.size())
		return (0);
	if (this->conn->req_body_status() >= 0)
		return (0);
		
	return (-1);
}

int		FcgiPipe::hup(void)
{
	return (-1);
}

void	FcgiPipe::rsrc_closed(void)
{ 
	this->conn = NULL;
	this->rsrc = NULL;
}



