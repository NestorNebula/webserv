/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceFcgi.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:12:39 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/01 19:06:17 by kdonlon          ###   ########.fr       */
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
		
	if (!this->hed) //  && this->fcgi)
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

// #if !RES_CGI_WAIT_COMPLETE
	// have data, not waiting for complete
	// caller may flush RESP
	if (!this->wait_comp && this->resp_data()) // resp.size())
		return (1);
// #endif
	
	if (this->wait(0) != -1) // exited -- different for fcgi
	// if (this->done == RSRC_DONE_IO)
	{
		// this->set_done(RSRC_DONE_IO);// much worse ... 
		// this->wait(0);
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (RSP_ERROR);
			
		// truly done : should we be calling
		// chk_rsp_len here (?)
		// cgi does this in wait (!)
		// we do this in wait as well 
// #if 1 // RES_CGI_WAIT_COMPLETE
		// truly complete .. but data to be flushed
        if (this->resp_data()) // resp.size()) // resp_data()
            return (1);
// #endif
		if (this->ka)
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
// !!!
// uncertain : 
// more apt to have failed transactions 
// with no error 
// when major nofile attacks
	if (this->done == RSRC_DONE_IO)
	{
		WSLOG(LVL_DBG, TGT_FCGI, "wait:  (done)");
		if (this->hed == 0)
		{
			this->set_err(603); // CGI_ERR
			return (0);
		}
// #if RES_CGI_WAIT_COMPLETE
		// truly complete .. if wait_comp, check for setting content-length and keep-alive headers if necessary
		if (this->wait_comp)
			this->chk_rsp_len();
// #endif
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
		// WSLOG(LVL_DBG, TGT_FCGI, "rem");
		// WSLOG(LVL_DBG, TGT_FCGI, "done ", this->done);
		this->set_done(RSRC_DONE_IO);
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