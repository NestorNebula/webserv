/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 17:22:15 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "Connection.hpp"
#include "Server.hpp"


CgiPipe::CgiPipe (int _fd, Connection & _conn) : EpollClient(EPC_CGI, _fd), conn(_conn)
{
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
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
		return (-1);
	}

	// reading back from CGI -- 
	// ONLY "Content-Length" bytes
	// otherwise .. EOF
	// (!) BINARY DATA (!)

// Q: COMMUNICATION : between cgi/conn
	this->conn.ostr += std::string(this->ibuf);
    
	WsLog::_(LVL_INFO, TGT_CGI_RECV, "ostr");
	WsLog::_(LVL_INFO, TGT_CGI_RECV, "\n", this->conn.ostr);
	
    this->conn.serv.ep.mod(&this->conn, EPOLLOUT);

	return (err);
}

int		CgiPipe::pollout(void)
{
	int	err;

	this->timeout();
    
	WsLog::_(LVL_INFO, TGT_CGI_SEND, "send: ", this->conn.istr.size());

	// cgi : needs to have received CONTENT_LENGTH .. 
	// so it knows something is coming 
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
			return (-1);
		}
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
		this->conn.serv.ep.mod(this, 0);
		// close down .. 
		return (-1);

	}
	// this (fd) .. can WRITE
	// input TO CGI .. from conn.istr
	// WsLog::_(LVL_DBG, TGT_CGI_SEND, "pollout");
	
    // POLLOUT assumed .. 
    // mod : add/remove a certain flag

	// check : conn->istr

		// nothing more to write to cgi (?)
		// wait for more data 
	this->conn.serv.ep.mod(this, 0);
    return (0);
}
