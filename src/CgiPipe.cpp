/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 23:11:51 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "Connection.hpp"
#include "Server.hpp"


CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
}

CgiPipe::CgiPipe (int _fd, Connection & _conn) : EpollClient(EPC_CGI, _fd), conn(_conn)
{
}
	
int		CgiPipe::pollin(void)
{
	int	err = 0;

	this->timeout();
	
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: zero");
		// this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}

	this->istr += std::string(this->ibuf);
    
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "istr");
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);
		
	this->conn.ostr = this->istr;

    this->conn.serv.ep.mod(&this->conn, EPOLLOUT);

	return (err);
}

int		CgiPipe::pollout(void)
{
	this->timeout();
    
	// this (fd) .. can WRITE
	// input TO CGI .. from conn.istr
	// WsLog::_(LVL_DBG, TGT_CGI_SEND, "pollout");
	
	// this->conn.serv.ep.del(this);
	// this->state = EPC_STATE_SHUTDOWN;
    // POLLOUT assumed .. 
    // mod : add/remove a certain flag
	this->conn.serv.ep.mod(this, 0);
    return (0);
}
