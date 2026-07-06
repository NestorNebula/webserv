/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 21:14:34 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "Connection.hpp"
#include "Server.hpp"


CgiPipe::CgiPipe (Epoll &_ep, int _fd, Connection & _conn) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn)
{
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
}

ssize_t	CgiPipe::pollin(void)
{
	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: zero");
		return (0);
	}

	this->conn.ostr.append(this->ibuf, err);
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", conn.ostr.size());
    
	WsLog::_(LVL_INFO, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_INFO, TGT_CGI_DATA, "****\n", this->conn.ostr);
	
	// any chance we're reading (EPOLLIN) at the same time (?)

		// partial
    // this->conn.mod_evt(EPOLLOUT);

	return (err);
}

// cgi::hup .. set conn state DONE 
// epoll::state (read_data)
	// has read data from its (fd) 
	// that is avaiable for processing (in its ibuf)
	 
ssize_t	CgiPipe::pollout(void)
{
	ssize_t	err;
    
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send");

	// cgi : needs to have received CONTENT_LENGTH .. 
	// so it knows something is coming 
	// otherwise we need to CLOSE its INPUT
	// conn::state (read_data) 
	
	if (this->conn.istr.size())
	{
		err = this->send(this->conn.istr); // body
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
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
		return (0); // keep going
	}
	// ASSUMES : conn::input : is faster than our output 
	this->conn.mod_evt(EPOLLOUT);
	return (-1); // EOF : close input to cgi
}

int		CgiPipe::hup(void)
{
	// if POLLIN
		// set_state() could better protect/compare 
	this->conn.state = RSRC_SENT_BODY;
	this->conn.mod_evt(EPOLLOUT); // make sure it gets detected
	return (-1);
}