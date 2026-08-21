/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiPipe.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 02:32:54 by kdonlon          ###   ########.fr       */
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


// The server is in no way obligated to send end-of-file 
// after the script reads CONTENT_LENGTH bytes. 

ssize_t	FcgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
		
	switch(rsrc->get_req_body())
	{
	case REQ_WAIT_HEAD:
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "head     : waiting");
		this->mod_evt(0);
		return (0);
	case REQ_WAIT_BODY:
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body     : waiting");
		this->mod_evt(-EPOLLOUT);
		return (0);
	case REQ_COMPLETE: // still need to send END_STDIN
	default:
		break;
	}
		
	if (rsrc->body.size() == 0)
	{
		rsrc->set_done(RSRC_DONE_IP);
		this->mod_evt(-EPOLLOUT);
// ALL_BODY_SENT
		this->mod_evt(EPOLLIN); 
	}
	
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", rsrc->body.size());
	fcgi.req_body((char*) rsrc->body.c_str(), rsrc->body.size());
	rsrc->body.clear();

	err = this->send(fcgi.req);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "send");
		this->rsrc->set_err(500); // Internal Server Error
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_FCGI, "send:  ZERO");
		rsrc->set_done(RSRC_DONE_IP);
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_FCGI, "sent: ", err);
	WsLog::_(LVL_DBG, TGT_FCGI, "left: ", fcgi.req.size());

// SOME_BODY_SENT
	// this->mod_evt(EPOLLIN); 
	return (0);
}

ssize_t	FcgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	WsLog::_(LVL_DBG, TGT_FCGI, "recv:  POLLIN");
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
// HAVE_SOME_DATA
		// conn->mod_evt(EPOLLOUT);
		break;
	}
	
	fcgi.rsp.clear();
	return (err);
}

// epoll : evt tgt  : conn
// epoll : evt fd   : [7]
// epoll : evt typ  : in out rdhup err hup 
// conn  : hup!
// epoll : cli rem  : conn
// epoll : cli del  : conn
// conn  :  (~) Connection [7]
// conn  : req cnt: [1]
// rsrc  :  (~) ResourceFcgi
// epc   :  (~) EpollClient
// epoll : ecnt  : [1]
// epoll : 
// epoll : evt tgt  : fcgi
// epoll : evt fd   : [8]
// epoll : evt typ  : in rdhup 
// fcgi  : recv:  POLLIN
// fcgi  : recv
// epc   : read: [0]
// epc   : read:  ZERO
// fcgi  : recv: [0]
// fcgi  : recv:  ZERO
// fcgi  : req : [0]
// fcgi  : RDHUP
// fcgi  : rdhup: resp.size() [4776824]
// fcgi  : rdhup: error [0]
// ./mak.sh: line 24: 2102233 Segmentation fault         (core dumped) ./test "$CONF" "$1"

int		FcgiPipe::rdhup(void)
{
	WsLog::color(WSL_YELLOW);
	WsLog::_(LVL_DBG, TGT_FCGI, "RDHUP");
	if (this->rsrc == NULL)
		return (-1);
	if (this->conn == NULL)
		return (-1);
		
	if (this->fcgi.req.size())
	{
		WsLog::color(WSL_RED);
		WsLog::_(LVL_TMP, TGT_FCGI, "rdhup: req.size() ", this->fcgi.req.size());
		return (0);
	}
	// if (this->rsrc->error)
	// 	return (-1);
	if (this->rsrc->resp.size())
	{
		WsLog::color(WSL_YELLOW);
		WsLog::_(LVL_DBG, TGT_FCGI, "rdhup: resp.size() ", this->rsrc->resp.size());
		WsLog::_(LVL_DBG, TGT_FCGI, "rdhup: error ", this->rsrc->error);
		
// connection close .. by client
// rsrc->fcgi set to NULL
// when pipe hangs up -- output is done .. 
		this->rsrc->rem(this);
		if (this->conn)
			this->conn->mod_evt(EPOLLOUT);
		return (0);
	}
	if (this->conn)
		this->conn->mod_evt(EPOLLOUT);
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



