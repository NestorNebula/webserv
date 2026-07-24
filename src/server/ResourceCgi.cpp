/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 17:51:05 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceCgi.hpp"



ResourceCgi::~ResourceCgi()
{
	WsLog::_(LVL_DBG, TGT_RSRC, "(~) ResourceCgi");
	this->reset();
}

void	ResourceCgi::reset(void)
{
	WsLog::_(LVL_DBG, TGT_RSRC, "reset");
	if (this->ip || this->op) // pid, stat (?)
	{
		if (this->stat == -1)
		{
			WsLog::_(LVL_DBG, TGT_RSRC, "kill");
			kill(this->pid, SIGKILL);
			this->status(0);
		}
	}
	if (this->ip)
	{
		this->ip->rsrc_closed(); // rsrc_closed
		// this->ip->mod_evt(EPOLLIN);
	}
	if (this->op)
	{
		this->op->rsrc_closed();
		// this->op->mod_evt(EPOLLOUT);
	}
	this->pid  = 0;
	this->ip   = NULL;
	this->op   = NULL;
	this->stat = -1;
	this->hed  = 0;
	this->clen = 0;
	this->hlen = 0;
	this->tlen = -1;
	this->slen = 0;
	this->ka   = 0;
}

int	ResourceCgi::status(int opt)
{
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "pid : ", this->pid);
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "xit : ", this->xit);
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "stat: ", this->stat);
	if (this->stat != -1)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (this->pid == 0)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	
	int err = waitpid(this->pid, &this->stat, opt);
	
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG => no change => (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
		switch (this->xit)
		{
		case 0:
			break;
		case 2:
			this->set_err(404);
			break;
		default:
			this->set_err(504);
			break;
		}
		WsLog::_(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "exit: ", xit);
		// valgrind : "Unknown error 255" is malloc'ed (!)
		if (xit < 255)
			WsLog::_(LVL_DBG, TGT_RSRC, "exit:  ", std::strerror(xit));
		else
			WsLog::_(LVL_DBG, TGT_RSRC, "exit:  unknown");
	}
	else if (WIFSIGNALED(stat))
	{
		this->sig = WTERMSIG(stat);
		this->set_err(504);
		WsLog::_(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "sig : ", sig);
		WsLog::_(LVL_DBG, TGT_RSRC, "sig : ", strsignal(sig));
	}
	else
	{
		WsLog::_(LVL_INFO, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "STAT: ", stat);
	}
	this->pid = 0;
	return (this->stat);
}

// ~CgiPipe
int	ResourceCgi::rem(CgiPipe *epc)
{
	int err = 0;

	if (epc == this->ip)
	{
		err = 1;
		this->ip = NULL;
		if (this->op)
			this->op->mod_evt(EPOLLIN);
	}
	else if (epc == this->op)
	{
		err = 2;
		this->op = NULL;
	}
	if (this->ip == NULL && this->op == NULL)
		this->status(0);
		
	return (err);
}

void    ResourceCgi::push_data(void)
{
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
}
int	ResourceCgi::init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn)
{
	int	err;

	this->reset();

	this->pid = _pid;
// fcntl .. F_SETFD .. O_CLOEXEC
	WsLog::_(LVL_DBG, TGT_RSRC, "exec cgi");

	// ResourceCgi::init(conn, pipes, ep)
	int cgifd_ip = dup(pipes->p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)");

	int cgifd_op = dup(pipes->p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)");
	}	

	// (rsrc)
	this->ip = new CgiPipe(ep, cgifd_ip, conn, this);
	err = this->ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->op = new CgiPipe(ep, cgifd_op, conn, this);
	err = this->op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	return (err);	
}