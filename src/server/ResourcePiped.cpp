/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourcePiped.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:16:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/29 18:06:05 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourcePiped.hpp"

ResourcePiped::~ResourcePiped()
{
	WSLOG(LVL_DBG, TGT_RSRC, " (~) ResourceCgi");
	WSLOG(LVL_DBG, TGT_RSRC, "stat: " , this->stat);
	WSLOG(LVL_DBG, TGT_RSRC, "pid : " , this->pid);
	
	try
	{	
		this->conn_closed();
		this->conn = NULL;
		this->wait(WNOHANG);
		if (this->stat == -1 && this->pid)
		{
			WSCOL(WSL_RED);
			WSLOG(LVL_DBG, TGT_RSRC, "kill");
			kill(this->pid, SIGKILL);
			this->wait(0); // do not set error
		}
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CGI, " (~) ResourceCgi\n", e.what());
	}
}

void	ResourcePiped::conn_closed(void)
{
	if (this->ip)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "conn-closed : ip");
		this->ip->rsrc_closed();
		this->ip->mod_evt(EPOLLOUT);
	}
	if (this->op)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "conn-closed : op");
		this->op->rsrc_closed();
		this->op->mod_evt(EPOLLIN);
	}
}

int	ResourcePiped::status(void)
{
	if (this->error)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (RSP_ERROR);
	}
	if (!this->hed && this->ip)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		this->ip->mod_evt(EPOLLOUT);		
		return (RSP_WAIT_HEAD);
	}

#if !RES_CGI_WAIT_COMPLETE
	if (this->resp.size())
		return (1);
#endif
	if (this->wait(WNOHANG) != -1)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
		    return (RSP_ERROR);
			
// #if 1 // RES_CGI_WAIT_COMPLETE
        if (this->resp.size())
            return (1);
// #endif
		if (this->ka)
			return (RSP_KPALIVE);
		return (RSP_COMPLETE); 
	}
	
	WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	if (this->op)
		this->op->mod_evt(EPOLLIN);
	return (RSP_WAIT_BODY); 
}

int	ResourcePiped::wait(int opt)
{
	int	err;
	
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "pid : ", this->pid);
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "xit : ", this->xit);
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "stat: ", this->stat);

	if (this->stat != -1)
	{
		WSLOG(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (this->pid == 0)
	{
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (opt && (this->ip || this->op))
	{
		WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: nohang i/o");
		return (this->stat); // (-1) : still active
	}
	// aha : opt (!)
	err = waitpid(this->pid, &this->stat, opt);
	
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: ", err);
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG => no change => (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
		WSLOG(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "exit: ", xit);
		// valgrind : "Unknown error 255" is malloc'ed (!)
		if (xit < 255)
		{
			WSLOG(LVL_DBG, TGT_RSRC, "exit:  ", std::strerror(xit));
		}
		else
		{
			WSLOG(LVL_DBG, TGT_RSRC, "exit:  unknown");
		}
	}
	else if (WIFSIGNALED(stat))
	{
		this->sig = WTERMSIG(stat);
		WSLOG(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "sig : ", sig);
		WSLOG(LVL_DBG, TGT_RSRC, "sig : ", strsignal(sig));
	}
	else
	{
		WSLOG(LVL_INFO, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "STAT: ", stat);
	}
	// hm : forced-wait .. should not set error (?)
	if ((this->stat > 0) || (this->hed == 0))
		this->set_err(500);
#if RES_CGI_WAIT_COMPLETE
	else
		this->chk_rsp_len();
#endif
	this->pid = 0;
	return (this->stat);
}



int	ResourcePiped::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->ip)
	{
		WSLOG(LVL_INFO, TGT_RSRC, "rem : (ip)");
		err = 1;
		this->ip = NULL;
		if (this->op)
        {
            WSLOG(LVL_INFO, TGT_RSRC, "mod : (op)");
			this->op->mod_evt(EPOLLIN);
        }
	}
	else if (epc == this->op)
	{
		WSLOG(LVL_INFO, TGT_RSRC, "rem : (op)");
		err = 2;
		this->op = NULL;
	}
	if ((this->ip == NULL) && (this->op == NULL))
	{
		WSLOG(LVL_INFO, TGT_RSRC, "rem : (done)");
		err = 3;
		this->wait(WNOHANG);
	}	
	return (err);
}




void    ResourcePiped::push_body(void)
{
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
}



int	ResourcePiped::init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn)
{
	int	err;
	
	WSLOG(LVL_DBG, TGT_RSRC, "init:  PIPE");
	
	this->pid = _pid;
	
	int cgifd_ip = dup(pipes->p1[1]);
	if (cgifd_ip < 0)
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));

	int cgifd_op = dup(pipes->p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));
	}	
	
	this->ip = new CgiPipe(ep, cgifd_ip, conn, this);
	err = this->ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->op = new CgiPipe(ep, cgifd_op, conn, this);
    // epoll : mod_evt  : not yet initialized
	// err = this->op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	this->conn = conn;
	return (err);
}