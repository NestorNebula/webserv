/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 17:25:15 by kdonlon          ###   ########.fr       */
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

int		CgiPipe::pollin(void)
{
	int	err = 0;
	
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
		return (-1);
	}

	WsLog::_(LVL_INFO, TGT_CGI_RECV, "ibuf");
	WsLog::_(LVL_INFO, TGT_CGI_RECV, "****\n", this->ibuf);

	// reading back from CGI -- 
	// ONLY "Content-Length" bytes
	// otherwise .. EOF
	// (!) BINARY DATA (!)

// Q: COMMUNICATION : between cgi/conn
// or .. simply ensure 
	// ibuf = conn::rsrc.data_dst

	// UGLY
// or : conn checks ... cgi_out::state (read_data)

// EPC_OUT_SIZ

// read .. INTO .. conn::obuf (?)
	this->conn.ostr += std::string(this->ibuf);
    
	WsLog::_(LVL_INFO, TGT_CGI_RECV, "ostr");
	WsLog::_(LVL_INFO, TGT_CGI_RECV, "****\n", this->conn.ostr);
	
	// any chance we're reading (EPOLLIN) at the same time (?)

// this is the conn/cgi COMMUNICATION
// what is the best way to do this (?)
// OR : conn .. just checks if data in cgi::ibuf ... 
// rsrc
    this->conn.mod_evt(EPOLLOUT);

	return (err);
}

// epoll::state (read_data)
	// has read data from its (fd) 
	// that is avaiable for processing (in its ibuf)
	 
int		CgiPipe::pollout(void)
{
	int	err;
    
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
			return (-1);
		}
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
		
		this->conn.mod_evt(0); // probably a bad idea
		// just "turn off" here .. 
		// or .. continue 
		// close down .. 
		return (-1);
	}
	this->mod_evt(0);
	return (0);
	
// epoll : evt typ  : out 
// epoll : evt tgt  : cgi
// epoll : evt fd   : [7]
// cgi   : send: [0]
// epoll : mod cli  : serv // !!!!!

	// this (fd) .. can WRITE
	// input TO CGI .. from conn.istr
	// WsLog::_(LVL_DBG, TGT_CGI_SEND, "pollout");
	
    // POLLOUT assumed .. 
    // mod : add/remove a certain flag

	// NOT NECESSSARILY 
	// conn .. make be continuing to receive input .. to be passed to cgi
	return (-1);

		// nothing more to write to cgi (?)
		// wait for more data 
	// RIGHT -- conn may no longer exist 
	// this->conn.mod_evt(0); // may want to preserve its POLLIN (!)
    return (0);
}
