/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiPipe.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/20 11:13:51 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FcgiPipe.hpp"
#include "ResourceCgi.hpp"

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
	WsLog::_(LVL_DBG, TGT_FCGI, " (~) Fcgi");
	if (this->conn)
		this->conn->cgi_rem(this);
}

static int to = 0;
bool	FcgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CGI_TIMEOUT) > now)
		return (false);
	
// tricky .. CGI at (2s)

	if (this->rsrc)
	{
		WsLog::_(LVL_TMP, TGT_FCGI, "TIMEO : rsrc");
		Session &sess = conn->sess;
		Request &req  = sess.getRequest(); // Wrong Action on Session
		if (req.hasHeaders())
			WsLog::_(LVL_TMP, TGT_FCGI, "req : has headers");
		if (req.hasBody())
			WsLog::_(LVL_TMP, TGT_FCGI, "req : has body");
		if (req.isComplete())
		{
			WsLog::_(LVL_TMP, TGT_FCGI, "req : complete ", ++to); // 1500 (!)
			// sess body .. could be in between
#if 0
			// if (this == this->rsrc->ip)
			{
				this->lact = now;
				this->mod_evt(-EPOLLOUT);
// so .. timeout .. on an event we are not waiting for (?)
// not the same as cgi pipe ... 
				// this->rsrc->rem(this); // Wrong action on session
// EX: main() : Wrong action on Session
// pure virtual method called
// terminate called without an active exception

				return (false);
			}
#endif
		}
		// may still need to receive .. 
		this->rsrc->set_err(504); // Gateway Timeout
	}
	else if (this->conn)
	{
		WsLog::_(LVL_TMP, TGT_FCGI, "TIMEO : conn");
		this->conn->set_err(504); // Gateway Timeout
	}
	else
	{
		WsLog::_(LVL_TMP, TGT_FCGI, "TIMEO : ????");
		// why doesn't return (true) kill it 
		// but .. no (conn) anymore 
		// this->conn->set_err(504); // CGI_ERR
	}
	return (true);
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
		// WsLog::_(LVL_DBG, TGT_FCGI, "body: ", this->conn->req_body_status());
		rsrc->set_done(RSRC_DONE_OP);
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

ssize_t	FcgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
	
	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT: ", fcgi.req.size());
	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT\n", fcgi.req);
// WEBSERV : REQUEST (body)
	Session &sess = conn->sess;
	Request &req  = sess.getRequest();

#if 1
	if (!req.hasHeaders())
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "head     : waiting");
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	// ATTN : UPLOADS
	if (req.hasBody())
	{
		std::string & body = req.get_body();
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", body.size());
		fcgi.req_body((char*) body.c_str(), body.size());
	}
	else if (req.isComplete())
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body     : done (?)");
		fcgi.req_body(NULL, 0);
		rsrc->set_done(RSRC_DONE_IP);
	}
	else
	{
		rsrc->set_done(RSRC_DONE_IP);
		return (0);
	}
#else
	err = this->conn->req_body_status();


	if (err > 0)
	{
		// std::string & body = this->conn->sess.req.get_body();
// BUIDL_DEMO
		std::string body;
		WsLog::_(LVL_DBG, TGT_FCGI, "body: ", body.size());

		fcgi.req_body((char*) body.c_str(), body.size());
		body.clear(); 
	}
#endif



	err = this->send(fcgi.req);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "send");
		this->rsrc->set_err(500); // Internal Server Error
		return (err);
	}
	if (err == 0)
	{
		WsLog::color(WSL_GREEN);
		WsLog::_(LVL_DBG, TGT_FCGI, "send:  ZERO");
		rsrc->set_done(RSRC_DONE_IP);
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_FCGI, "sent: ", err);
	WsLog::_(LVL_DBG, TGT_FCGI, "left: ", fcgi.req.size());

	this->mod_evt(EPOLLIN); 
#if 0 // still need to send NULL (?)
	if (req.isComplete())
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body     : complete");
		this->mod_evt(-EPOLLOUT);
		// this->mod_evt(EPOLLIN);
		return (0);
	}
#endif 
	return (0);
}

int		FcgiPipe::rdhup(void)
{
	// nothing more to "send back"
	// but .. still may be receiving an upload
	// this->mod_evt(-EPOLLIN); // BAD IDEA
	// this->mod_evt(-EPOLLOUT);
	WsLog::color(WSL_YELLOW);
	WsLog::_(LVL_DBG, TGT_FCGI, "RDHUP");
	if (rsrc == NULL)
		return (-1);
	// CLEAN (!)
	return (rsrc->set_done(RSRC_DONE_IP));
	
// THE QUESTION : when to die 
#if 1
	if (this->fcgi.req.size())
	{
		WsLog::color(WSL_RED);
		WsLog::_(LVL_TMP, TGT_FCGI, "rdhup: req.size()");
		return (0);
	}
#endif
// we may (rdhup) 
// but can't CLOSE until BOTH SIDES ARE DONE 
// STATE CHECK
// find TEST CASES
	// still need to send BODY_DONE 
	// may be error (?)
	if (this->rsrc->resp.size())
	{
		WsLog::color(WSL_YELLOW);
		WsLog::_(LVL_DBG, TGT_FCGI, "rdhup: resp.size()");
		WsLog::_(LVL_DBG, TGT_FCGI, "rdhup: error ", this->rsrc->error);
		// THIS REALLY HELPED
		this->conn->mod_evt(EPOLLOUT);
		return (0);
	}
	this->conn->mod_evt(EPOLLOUT);
	return (-1);
// #if 1
// 	if (this->conn->req_body_status() >= 0)
// 	{
		
// 		WsLog::color(WSL_GREEN);
// 		WsLog::_(LVL_TMP, TGT_FCGI, "rdhup: body status");
// 		return (0);
// 	}
// #endif
// 	return (-1);
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



