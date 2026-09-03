/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceFcgi.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:12:39 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 21:29:42 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceFcgi.hpp"
#include "Server.hpp"

ResourceFcgi::~ResourceFcgi()
{
	WSLOG(LVL_DBG, TGT_RSRC, " (~) ResourceFcgi");
	this->conn_closed();
}

int	ResourceFcgi::init(Epoll *ep, CgiEnv *cgienv, Connection *conn, std::string &sock_path)
{	
	int fd = FcgiConn::make_sock(sock_path);
	if (fd < 0)
		return (-1);
	
	this->fcgi = new FcgiPipe(ep, fd, conn, this);
	int err = this->fcgi->init(cgienv);
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

void    ResourceFcgi::push_body(void)
{
	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLOUT);
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
		
	if (!this->hed)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		if (this->fcgi)
		{
			this->fcgi->mod_evt(EPOLLOUT);	
			return (RSP_WAIT_HEAD);
		}
		else
		{
			this->set_err(602); // CGI_ERR
			return (RSP_ERROR);
		}
	}

	if (!this->wait_comp && this->resp_data()) 
		return (1);
	
	if (this->wait(0) != -1)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (RSP_ERROR);

        if (this->resp_data())
		{
			WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (have data)");
            return (1);
		}
		if (this->ka) // DONE
			return (RSP_KPALIVE);
		return (RSP_COMPLETE); 
	}
	// STILL RUNNING
	WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLIN);
	return (RSP_WAIT_BODY); 
}

int	ResourceFcgi::wait(int opt)
{
	(void)opt;
	
	if (this->done & RSRC_DONE_ERR)
	{
		WSLOG(LVL_DBG, TGT_FCGI, "wait:  (error)");
		return (0);
	}
	if (this->done & RSRC_FLUSHING)
	{
		// WSLOG(LVL_DBG, TGT_FCGI, "wait:  (flush)");
		return (0);
	}
	if (this->done == RSRC_DONE_IO)
	{
		WSLOG(LVL_DBG, TGT_FCGI, "wait:  (done)");
		if (this->hed == 0)
		{
			this->set_err(603); // CGI_ERR
			return (0);
		}
		if (this->wait_comp)
			this->chk_rsp_len();
		this->set_done(RSRC_FLUSHING);
		return (0);
	}
	return (-1);
}

int	ResourceFcgi::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->fcgi)
	{
		// WSCOL(WSL_CYAN);
		// WSLOG(LVL_DBG, TGT_FCGI, "rem  ", this->done);
		this->set_done(RSRC_DONE_IO);
		this->fcgi = NULL;
		err = RSRC_DONE_IO;
	}
	return (err);
}
