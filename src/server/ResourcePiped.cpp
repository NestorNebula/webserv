/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourcePiped.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:16:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/04 09:31:37 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourcePiped.hpp"

ResourcePiped::~ResourcePiped()
{
	WSLOG(LVL_DBG, TGT_RSRC, " (~) ResourcePiped");
	WSLOG(LVL_DBG, TGT_RSRC, "stat: " , this->stat);
	WSLOG(LVL_DBG, TGT_RSRC, "pid : " , this->pid);
	
	try
	{	
		this->conn_closed();
		this->conn = NULL;
		if (!this->failed)
			this->wait(WNOHANG);
		if (this->stat == -1 && this->pid)
		{
			WSCOL(WSL_RED);
			WSLOG(LVL_DBG, TGT_RSRC, "kill");
			kill(this->pid, SIGKILL);
			this->wait(0);
		}
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CGI, " (~) ResourcePiped\n", e.what());
	}
}

int	ResourcePiped::init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn)
{
	int	err;

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_RSRC, "init:  PIPE");
	
	this->pid = _pid;
	
	int cgifd_ip = dup(pipes->p1[1]);
	if (cgifd_ip < 0)
	{
		this->set_failed();
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));
	}
	int cgifd_op = dup(pipes->p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		this->set_failed();
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));
	}	

	this->ip = new CgiPipe(ep, cgifd_ip, conn, this);
	err = this->ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		this->set_failed();
		return (err);
	}

	this->op = new CgiPipe(ep, cgifd_op, conn, this);
	// err = this->op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		this->set_failed();
		return (err);
	}
	this->conn = conn;
	return (err);
}

void    ResourcePiped::push_body(void)
{
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
}

void	ResourcePiped::conn_closed(void)
{
	if (this->ip)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "rsrc-closed : ip");
		this->ip->rsrc_closed();
		this->ip->mod_evt(EPOLLOUT);
	}
	if (this->op)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "rsrc-closed : op");
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

	if (!this->wait_comp && this->resp_data())
		return (1);
	
	if (this->wait(WNOHANG) != -1)
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
		WSLOG(LVL_DBG, TGT_RSRC_INFO, "STAT: ", this->stat);
		return (this->stat);
	}
	if (this->pid == 0)
	{
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_RSRC_INFO, "PID : ", this->stat);
		return (this->stat);
	}

	if (opt && (this->ip || this->op))
	{
		WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: nohang i/o ", this->failed);
		return (this->stat); // (-1) : still active
	}
	
	if ((this->ip == NULL) && (this->op == NULL))
	{
		WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: null i/o ", this->failed);
		opt = 0;
	}
	err = waitpid(this->pid, &this->stat, opt);
	
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: ", err);
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG // (-1) : still active
		
	this->pid = 0;
	
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_RSRC, "waitpid");
		
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
		WSLOG(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "exit: ", xit);
		
		if (xit < 255)
		{
			WSLOG(LVL_DBG, TGT_RSRC, "exit:  ", std::strerror(xit));
		}
		else
		{
			// valgrind : "Unknown error 255" is malloc'ed (!)
			WSLOG(LVL_DBG, TGT_RSRC, "exit:  unknown");
		}
	}
	else if (WIFSIGNALED(stat))
	{
		this->sig = WTERMSIG(stat);
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "sig : ", sig);
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_RSRC, "sig : ", strsignal(sig));
		this->set_err(500); // #kd (604)
		return (this->stat);
	}
	else
	{
		WSLOG(LVL_INFO, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "STAT: ", stat);
	}
	if (this->hed == 0)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "wait : error: 605");
		WSLOG(LVL_DBG, TGT_RSRC, "stat : ", stat);
		// WSLOG(LVL_DBG, TGT_RSRC, "req:\n", this->body);
		// WSLOG(LVL_DBG, TGT_RSRC, "rsp:\n", this->resp);

		// do not set error if we are going to retry
		if (this->stat || (this->conn && !this->conn->retry_cgi))
		{
			// WSLOG(LVL_TMP, TGT_RSRC_WAIT, "hed : ", 0);
			// WSLOG(LVL_TMP, TGT_RSRC_WAIT, "stat: ", stat);
			this->set_err(500); // #kd (605)
		}
	}
	else if (this->wait_comp)
		this->chk_rsp_len();
	return (this->stat);
}



int	ResourcePiped::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->ip)
	{
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (ip)");
		err = RSRC_DONE_IP;
		this->ip = NULL;
		if (this->op)
        {
            WSLOG(LVL_INFO, TGT_RSRC, "mod : (op)");
			this->op->mod_evt(EPOLLIN);
        }
	}
	else if (epc == this->op)
	{
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (op)");
		err = RSRC_DONE_OP;
		this->op = NULL;
	}
	if ((this->ip == NULL) && (this->op == NULL))
	{
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (done)");
		err = RSRC_DONE_IO;
		this->wait(0);
	}	
	return (err);
}

void    ResourcePiped::set_failed(void)
{
	this->failed = true;
	kill(this->pid, SIGKILL);
}
