/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceFcgi.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:12:39 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 02:28:41 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceFcgi.hpp"
#include "Server.hpp"

ResourceFcgi::~ResourceFcgi()
{
	WsLog::_(LVL_DBG, TGT_RSRC, " (~) ResourceFcgi");
	this->conn_closed();
}

void	ResourceFcgi::conn_closed(void)
{
	if (this->fcgi)
		this->fcgi->rsrc_closed();
}


int	ResourceFcgi::status(void)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (RSP_ERROR);
	}
		
	if (!this->hed && this->fcgi)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		this->fcgi->mod_evt(EPOLLOUT);	
		return (RSP_WAIT_HEAD);
	}

// HAVE_SOME_DATA
	// if (this->resp.size())
	// 	return (1);
	
	if (this->fcgi == NULL)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (RSP_ERROR);
// HAVE_ALL_DATA
        if (this->resp.size())
            return (1);
		return (RSP_COMPLETE); // RSP_COMPLETE
	}
	// STILL RUNNING
	WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	this->fcgi->mod_evt(EPOLLIN);
	return (RSP_WAIT_BODY); 
}
int	ResourceFcgi::wait(int opt)
{
	(void)opt;
	if (this->fcgi)
		return (-1);
	return (0);
}
int	ResourceFcgi::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->fcgi)
	{
		this->fcgi = NULL;
		err = 3;
	}
	return (err);
}
void    ResourceFcgi::push_body(void)
{
	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLOUT);
}
int	ResourceFcgi::init(Epoll *ep, CgiEnv *cgienv, Connection *conn)
{	
	int err;

	WsLog::_(LVL_DBG, TGT_RSRC, "init:  FCGI");

// WEBSERV : SERVER
	int fd = FcgiConn::make_sock(conn->serv.fcgi_sock.c_str());
	if (fd < 0)
		return (-1);
	
	this->fcgi = new FcgiPipe(ep, fd, conn, this);
	err = this->fcgi->init(cgienv);
	if (err < 0)
	{
		delete (this->fcgi);
		this->fcgi = NULL;
		close(fd);
		return (err);
	}
	this->fcgi->ini_evt(EPOLLOUT);
	this->conn = conn;
	return (err);
}