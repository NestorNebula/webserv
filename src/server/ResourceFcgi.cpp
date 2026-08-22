/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceFcgi.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:12:39 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/22 13:05:10 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceFcgi.hpp"
#include "Server.hpp"

ResourceFcgi::~ResourceFcgi()
{
	WSLOG(LVL_DBG, TGT_RSRC, " (~) ResourceFcgi");
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
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (RSP_ERROR);
	}
		
	if (!this->hed && this->fcgi)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		this->fcgi->mod_evt(EPOLLOUT);	
		return (RSP_WAIT_HEAD);
	}

#if !RES_CGI_WAIT_COMPLETE
	if (this->resp.size())
		return (1);
#endif
	
	if (this->wait(0) != -1) // exited
	// if (this->done == RSRC_DONE_IO)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (RSP_ERROR);
#if RES_CGI_WAIT_COMPLETE
        if (this->resp.size())
            return (1);
#endif
		return (RSP_COMPLETE); // RSP_COMPLETE
	}
	// STILL RUNNING
	WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	this->fcgi->mod_evt(EPOLLIN);
	return (RSP_WAIT_BODY); 
}
int	ResourceFcgi::wait(int opt)
{
	(void)opt;
	if (this->done & RSRC_DONE_ERR)
		return (0);
	if (this->done == RSRC_FLUSHING)
		return (0);
	if (this->done == RSRC_DONE_IO)
	{
#if RES_CGI_WAIT_COMPLETE
		this->chk_rsp_len();
#endif
		this->done = RSRC_FLUSHING;
		return (0);
	}
	if (this->fcgi)
		return (-1); // continue
	return (0);
}
int	ResourceFcgi::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->fcgi)
	{
		// set_done()
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

	WSLOG(LVL_DBG, TGT_RSRC, "init:  FCGI");
		// should have been checked before calling
	if (conn->serv.get_conf().fcgi_sock.empty())
	{
		WSLOG(LVL_DBG, TGT_CGI, "fcgi_sock: empty");
		return (-1);
	}
// WEBSERV : SERVER
	int fd = FcgiConn::make_sock(conn->serv.get_conf().fcgi_sock.c_str());
	if (fd < 0)
	{
		WSLOG(LVL_DBG, TGT_CGI, "fcgi_sock: FAIL");
		return (-1);
	}
	
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