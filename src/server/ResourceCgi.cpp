/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/31 17:52:02 by kdonlon          ###   ########.fr       */
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
	WsLog::_(LVL_DBG, TGT_RSRC, "(~) ResourceCgi");
	this->shutdown();
}

int ResourceCgi::shutdown(void)
{
	this->status(WNOHANG);
	if (this->stat == -1 && this->pid)
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "kill");
		kill(this->pid, SIGKILL);
		this->status(0); // dangerous (?)
	}
	this->conn_closed();
	return (this->stat);
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
	
	err = waitpid(this->pid, &this->stat, opt);
	
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
	{
		err = 3;
		this->status(WNOHANG);
	}	
	return (err);
}


int		ResourceCgi::chk_rsp_hed(std::string & ostr)
{
	size_t	pos = ostr.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (0);
		
// rsrc::parse_head
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	this->hed = 1;
	this->hlen = pos + 4;
	
// std::string stat;
// std::string head;
// std::string body; // (ostr)
	std::string stat_val = hedval_str(ostr, "Status");
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_val);
	if (stat_val.size())
	{
		int http_stat = atoi(stat_val.c_str());
		if (http_stat != 200)
		{
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "STAT: ", http_stat);
			this->error = http_stat;
			return (0);
		}		
	}

	std::string conn_close("Connection: close\r\n");
	std::string conn_keep("Connection: keep-alive\r\n");
	
// PHP Warning:  PHP Request Startup: POST Content-Length of 14976177 bytes exceeds the limit of 8388608 bytes in Unknown on line 0
	std::string conn_val = hedval_str(ostr, "Content-Length");
	if (conn_val.size())
	{
		this->clen = atoi(conn_val.c_str());
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
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "conn: error ", this->error);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "cgi : error ", this->error);
		this->ka = 0;
		ostr.insert(0, conn_close);
	}
	
	std::string stat_200("HTTP/1.0 200 OK\r\n");
	ostr.insert(0, stat_200);
	this->tlen += stat_200.size();
	
	// WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", OSTR);	
	return (0);
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
	return (err);	
}