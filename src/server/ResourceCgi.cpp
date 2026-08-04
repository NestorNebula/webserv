/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/04 19:26:55 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceCgi.hpp"



// It is not currently possible to reliably delete epoll items when using the same epoll set from multiple threads. After calling epoll_ctl with EPOLL_CTL_DEL, another thread might still be executing code related to an event for that epoll item (in response to epoll_wait). Therefore the deleting thread does not know when it is safe to delete resources pertaining to the associated epoll item because another thread might be using those resources. 

// HM : do not delete ..CgiPipe .. until .. we are sure (cgi) is complete (?)
// so .. keep them in the epoll .. but .. disabled

// the reading application would be tied up for a long period; 
// meanwhile, it does not service I/O events on the other file descriptors—those descriptors are starved of service by the application. 

// The solution to file descriptor starvation is for the application to maintain a user-space data structure that caches the readiness of each of the file descriptors that it is monitoring. 

// so .. cache as (READY) .. until .. recv/send ZERO ...

// EPOLLONESHOT. If this flag is specified in the events mask for a file descriptor, then, once the file descriptor becomes ready and is returned by a call to epoll_wait(), it is disabled from further monitoring (but remains in the interest list). If the application is interested in monitoring file descriptor once more, then it must re-enable the file descriptor using the epoll_ctl(EPOLL_CTL_MOD) operation. 


ResourceCgi::~ResourceCgi()
{
	WsLog::_(LVL_DBG, TGT_RSRC, " (~) ResourceCgi");
	
	// not sure this is the place ...
	// CgiPipe => hup => Epoll::rem => this
	this->status(WNOHANG);
	if (this->stat == -1 && this->pid)
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "kill");
		kill(this->pid, SIGKILL);
		this->status(0); // dangerous (?)
	}
	this->conn_closed();
}

void	ResourceCgi::conn_closed(void)
{
	if (this->ip)
	{
		this->ip->rsrc_closed();
		this->ip->mod_evt(EPOLLOUT);
	}
	if (this->op)
	{
		this->op->rsrc_closed();
		this->op->mod_evt(EPOLLIN);
	}
}

void	ResourceCgi::set_err(int e)
{
	this->error = e;
	if (this->conn)
		this->conn->set_err(e);
}

int	ResourceCgi::status(int opt)
{
	int	err;
	
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
	if (this->ip || this->op)
		return (this->stat); // (-1)
	
	(void)opt;
	err = waitpid(this->pid, &this->stat, 0); // opt);
	
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
			// (res)
			// do not override .. 
			// may be killing a successfully "finished" -- 
			// but not timed out
			// php 
			// does not (ka) nicely
			// because .. it SENDS VALID DATA (!)
			this->set_err(404);
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
		this->set_err(505); 
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

// ~CgiPipe - from Epoll (!)
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
	{
		err = 3;
		this->status(WNOHANG);
	}	
	return (err);
}

int		ResourceCgi::chk_rsp_hed(std::string & ostr)
{
	if (this->hed)
	{
		// conn->mod_evt(EPOLLOUT);
		return (RSRC_RESP_BODY);
	}	
	size_t	pos = ostr.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);
		
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	this->hed = 1;
	this->hlen = pos + 4;
	
	std::string conn_close("Connection: close\r\n");
	std::string conn_keep ("Connection: keep-alive\r\n");
	
	std::string conn_str = hedval_str(ostr, "Content-Length");
	if (conn_str.size())
	{
		this->clen = atoi(conn_str.c_str());
		this->tlen = this->hlen + this->clen;
		
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "hlen: ", this->hlen);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "clen: ", this->clen);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "tlen: ", this->tlen);
		
		if (this->ka)
		{
			ostr.insert(0, conn_keep);
			this->tlen += conn_keep.size();
		}
		else
		{
			ostr.insert(0, conn_close);
			this->tlen += conn_close.size();
		}
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "cgi : ka ", this->ka);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "conn: ka ", conn->ka);
		// but .. not .. conn .. which could over-ride if error 
		this->ka = 0; //  rsrc .. conn .. 
		ostr.insert(0, conn_close);
	}

	std::string stat_head;
	std::string stat_str = hedval_str(ostr, "Status");
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_str);
	if (stat_str.size())
	{
		// HTTP/1.1 STATUS [Status Messaage]
		stat_head = std::string("HTTP/1.0 ") + stat_str + "\r\n";
		// set error .. 
	}
	else
	{
		stat_head = std::string("HTTP/1.0 200 OK\r\n");
	}
	ostr.insert(0, stat_head);
	this->tlen += stat_head.size();

	// WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", this->ostr);	
	return (RSRC_RESP_HEAD);
}

int		ResourceCgi::recv_data(char *buf, int siz)
{
	this->ostr.append(buf, siz);
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", ostr);
	
	return (this->chk_rsp_hed(this->ostr));
}

void    ResourceCgi::push_body(void)
{
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
}

int	ResourceCgi::init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn)
{
	int	err;
	
	this->pid = _pid;
	
	WsLog::_(LVL_DBG, TGT_RSRC, "init cgi");

	int cgifd_ip = dup(pipes->p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)");

	int cgifd_op = dup(pipes->p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return WsLog::_errno(LVL_ERR, TGT_RSRC, "dup (pipes)");
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
	err = this->op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	this->conn = conn;
	return (err);	
}