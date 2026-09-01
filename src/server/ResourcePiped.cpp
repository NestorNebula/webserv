/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourcePiped.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:16:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/01 16:13:23 by kdonlon          ###   ########.fr       */
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
			// if (this->failed)
			// 	this->wait(WNOHANG);
			// else 
				this->wait(0); // do not set error (?)
		}
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CGI, " (~) ResourcePiped\n", e.what());
	}
}

void	ResourcePiped::conn_closed(void)
{
	if (this->ip)
	{
		WSLOG(LVL_DBG, TGT_RSRC, "rsrc-closed : ip");
		this->ip->rsrc_closed();
		// are these not falling over properly (?)
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

// #if !RES_CGI_WAIT_COMPLETE
	// have data, not waiting for complete
	// caller may flush RESP
	if (!this->wait_comp && this->resp_data()) // resp.size()) // resp_data
		return (1);
// #endif
	if (this->wait(WNOHANG) != -1)
	{
		WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
		    return (RSP_ERROR);
			
// #if 1 // RES_CGI_WAIT_COMPLETE
        if (this->resp_data()) // resp.size()) // resp_data
		{
			WSLOG(LVL_DBG, TGT_RSRC_STAT, "stat:  (have data)");
            return (1);
		}
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


	// hed check .. to set error (?)
	// may be done .. but deleted .. 
	// which seems to lead to timeout .. 

// what is NEW : we delete a FAILED (nofile) resource PREMATURELY
	
// still get a hang .. when deleting from conn
// not closed (?)
// waiting too long to close (fd) ?
// 
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
		// feels like we get a pause here sometimes .. 
		// may be killing (?)
		WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: nohang i/o ", this->failed);
		// seems to hold -- might be .. data
		return (this->stat); // (-1) : still active
	}
	// aha : opt (!)
	if ((this->ip == NULL) && (this->op == NULL))
	{
		WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: null i/o ", this->failed);
			// a lot of this, actually
	// are we getting this on a delete
	// when OPEN FILES failed (?)


	// Connection deleting .. 
// 	rsrc  : wait: null i/o
// rsrc  : dup (pipes)
// error : Too many open files
// rsrc  :  (~) ResourcePiped
// rsrc  : stat: [-1]
// rsrc  : pid : [303523]
// rsrc  : wait: null i/o


// we failed
// cgi is launched
// conn deletes RESOURCE
// pipes are still activate
// WAIT .. will wait forever 
// have not killed 
// first delete / no-hang call
// with both null
// waits forver
// without KILL

// TRY : close ip/op and let die (?)
// but : we do not have acess to those here 

// basic behavior NEEDS THIS 
// some clients (ip/op) are still floating around (?)

// set on REM

// was this linked to keep-alive (?)
		opt = 0;
		
// when did we REALLY need this (?)
// something about ... FILE FAIL -- delete ...
// and we're stuck waiting 
		if (false) // !this->conn)
		{
			// very heavy -- holds things up
			// when DELETE ON FAIL (!)
			WSCOL(WSL_RED);
			WSLOG(LVL_TMP, TGT_RSRC, "kill");
			kill(this->pid, SIGKILL);
			opt = 0; // dangerous (?) // wait on something that should be killed (?)
		}
	}
	err = waitpid(this->pid, &this->stat, opt);
	
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "wait: ", err);
	WSLOG(LVL_DBG, TGT_RSRC_WAIT, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG => no change => (-1)
		
	this->pid = 0;
	
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
		this->set_err(604); // CGI_ERR
		return (this->stat);
	}
	else
	{
		WSLOG(LVL_INFO, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "STAT: ", stat);
	}
	// hm : forced-wait .. should not set error (?)
	// if ((this->stat > 0) || (this->hed == 0))
	// stat test -- more retry fails 
	// more timeouts 
	if (this->stat == 0 && this->hed == 0) // allow exit to terminate
	// if (this->hed == 0)
	{
		// a lot on client close
		// the fail on low-file-limit 
		// killed (?)
		// deleting -- before retry (?)
		WSLOG(LVL_TMP, TGT_RSRC, "error: 605");
		WSLOG(LVL_TMP, TGT_RSRC, "stat : ", stat);
		WSLOG(LVL_TMP, TGT_RSRC, "req:\n", this->body);
		WSLOG(LVL_TMP, TGT_RSRC, "rsp:\n", this->resp);

		// not if RETRY
		if (this->conn && !this->conn->retry_cgi)
			this->set_err(605); // CGI_ERR
	}
// #if RES_CGI_WAIT_COMPLETE
	else if (this->wait_comp)
		this->chk_rsp_len();
// #endif
	return (this->stat);
}



int	ResourcePiped::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->ip)
	{
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (ip)");
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
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (op)");
		err = 2;
		this->op = NULL;
	}
	if ((this->ip == NULL) && (this->op == NULL))
	{
		// WSCOL(WSL_CYAN);
		WSLOG(LVL_DBG, TGT_RSRC, "rem : (done)");
		err = 3;
		this->wait(0); // WNOHANG);
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
	
	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_RSRC, "init:  PIPE");
	
	this->pid = _pid;
	
	int cgifd_ip = dup(pipes->p1[1]);
	if (cgifd_ip < 0)
	{
		// or : set_failed triggers (kill)
		this->failed = true;
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));
	}
	int cgifd_op = dup(pipes->p2[0]);
	if (cgifd_op < 0)
	{
		this->failed = true;
		close(cgifd_ip);
		return (WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)"));
	}	
	
	this->ip = new CgiPipe(ep, cgifd_ip, conn, this);
	err = this->ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		this->failed = true;
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->op = new CgiPipe(ep, cgifd_op, conn, this);
    // epoll : mod_evt  : not yet initialized
	// err = this->op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		this->failed = true;
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	this->conn = conn;
	return (err);
}