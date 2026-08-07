/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/07 11:45:27 by kdonlon          ###   ########.fr       */
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

// A defunct or "zombie" process in Linux occurs when a child process finishes running via fork(), but its parent process does not read its exit status using a wait() system call. The process remains in the table holding its PID until reaped.

ResourceCgi::~ResourceCgi()
{
#if RSRC_FCGI
		// NO : it is an EpollClient
	// if (this->fcgi)
	// 	delete (this->fcgi);
#else
	// WsLog::_(LVL_DBG, TGT_RSRC, " (~) ResourceCgi");
	WsLog::_(LVL_DBG, TGT_RSRC, "stat: " , this->stat);
	WsLog::_(LVL_DBG, TGT_RSRC, "pid : " , this->pid);
	
	// not sure this is the place ...
	// CgiPipe => hup => Epoll::rem => this
	// how did we get here .. without (stat) being set ... 
	this->wait(WNOHANG);
	if (this->stat == -1 && this->pid)
	{
		WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_RSRC, "kill");
			// no error .. nothing in output
		WsLog::_(LVL_DBG, TGT_RSRC, "err  : ", this->error);
		WsLog::_(LVL_DBG, TGT_RSRC, "ostr\n", this->ostr);
		// nothing here .. did it get reset (?)
		// WsLog::_(LVL_DBG, TGT_RSRC, "REQUEST\n", this->conn->sess.req.get_body());
		// kill(this->pid, SIGKILL); // => defunct => 
		// valgrind : connection reset by peer
		this->wait(0);
	}
#endif
	this->conn_closed();
}

void	ResourceCgi::conn_closed(void)
{
#if RSRC_FCGI
	if (this->fcgi)
		this->fcgi->rsrc_closed();
	return;
#else
// valgrind shit here with KEEP-ALIVE
	if (this->ip)
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "conn-closed : ip");
		this->ip->rsrc_closed();
		this->ip->mod_evt(EPOLLOUT);
	}
	if (this->op)
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "conn-closed : op");
		this->op->rsrc_closed();
		this->op->mod_evt(EPOLLIN);
	}
#endif
}

void	ResourceCgi::set_err(int e)
{
	this->error = e;
	if (this->conn)
		this->conn->set_err(e);
}

// NEED HEAD	0
// NEED_BODY	0
// HAVE_OSTR	1
// ERROR		2
// DONE			-1

int	ResourceCgi::consumed(int bytes)
{
	(void)bytes;

	return (0);
}

int	ResourceCgi::status(void)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (2);
	}
	
	if (!this->hed && this->ip)
	{
		WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		return (0); // NEED_HEAD
	}
	if (this->ostr.size())
		return (1);
	

#if RSRC_FCGI
	if (this->fcgi == NULL)
	{
#else
	if (this->wait(WNOHANG) != -1)
	{
#endif
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (2);
		return (-1);
	}
	WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

#if RSRC_FCGI
	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLIN);
#else
	if (this->op)
		this->op->mod_evt(EPOLLIN);
	if (this->ip)
		this->ip->mod_evt(EPOLLOUT);
#endif
	return (0); // NEED_DATA
}


// Header Safety: You must send HTTP headers (like Content-Type) before any body text. If an error occurs midway through generating output, buffering lets you discard the partial text and output a clean 500 Internal Server Error page instead of a broken, half-rendered HTML file.Content-Length: Holding the output lets you measure the exact byte size of your response so you can send an accurate Content-Length header.

int	ResourceCgi::wait(int opt)
{
	(void)opt;
// FCGI
#if RSRC_FCGI
	if (this->fcgi)
		return (-1);
	return (0);
#else
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
		WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (this->ip || this->op)
		return (this->stat); // (-1) : still active
	
	err = waitpid(this->pid, &this->stat, 0);
	
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_RSRC_WAIT, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG => no change => (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
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
		WsLog::_(LVL_DBG, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "sig : ", sig);
		WsLog::_(LVL_DBG, TGT_RSRC, "sig : ", strsignal(sig));
	}
	else
	{
		WsLog::_(LVL_INFO, (TGT_RSRC_WAIT | TGT_RSRC_INFO), "STAT: ", stat);
	}
	this->pid = 0;
	return (this->stat);
#endif
}

int	ResourceCgi::rem(EpollClient *epc)
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
	else if (epc == this->fcgi)
	{
		this->fcgi = NULL;
		return (3);
	}
	
	if (this->ip == NULL && this->op == NULL)
	{
		err = 3;
		this->wait(WNOHANG);
	}	
	return (err);
}

int		ResourceCgi::chk_rsp_hed(std::string & ostr)
{
	if (this->hed)
	{
		conn->mod_evt(EPOLLOUT);
		return (RSRC_RESP_BODY);
	}	
	size_t	pos = ostr.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);
		
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	this->hed = 1;
	
// require .. content-type (?)

	std::string conn_close("Connection: close\r\n");
	ostr.insert(0, conn_close);
	

	std::string stat_head;
	std::string stat_str = hedval_str(ostr, "Status");
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_str);
	if (stat_str.size())
	{
		// HTTP/1.1 STATUS [Status Message]
		stat_head = std::string("HTTP/1.0 ") + stat_str + "\r\n";
	}
	else
	{
		stat_head = std::string("HTTP/1.0 200 OK\r\n");
	}
	ostr.insert(0, stat_head);
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
#if RSRC_FCGI
	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLOUT);
#else
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
#endif
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

int	ResourceCgi::init(Epoll *ep, CgiEnv *cgienv, Connection *conn)
{	
	int err;

	std::string sock_path("/home/kdonlon/Documents/Projects/webserv/git/tst/server/FCGI/.php-fpm/SOCK");
	
	int fd = FcgiConn::make_sock(sock_path.c_str());
	if (fd < 0)
		return (-1);

	this->fcgi = new FcgiPipe(ep, fd, conn, this);
	err = this->fcgi->init(cgienv);
	if (err < 0)
	{
		delete (this->fcgi);
		this->fcgi = NULL;
		return (err);
	}
	this->fcgi->ini_evt(EPOLLOUT);

	WsLog::color(WSL_YELLOW);
	WsLog::_(LVL_TMP, TGT_CONN, "FCGI (!)");
	this->conn = conn;

	return (err);
}
