/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/20 22:56:56 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceCgi.hpp"
#include "Server.hpp"


// It is not currently possible to reliably delete epoll items when using the same epoll set from multiple threads. After calling epoll_ctl with EPOLL_CTL_DEL, another thread might still be executing code related to an event for that epoll item (in response to epoll_wait). Therefore the deleting thread does not know when it is safe to delete resources pertaining to the associated epoll item because another thread might be using those resources. 

// HM : do not delete ..CgiPipe .. until .. we are sure (cgi) is complete (?)
// so .. keep them in the epoll .. but .. disabled

// the reading application would be tied up for a long period; 
// meanwhile, it does not service I/O events on the other file descriptors—those descriptors are starved of service by the application. 

// The solution to file descriptor starvation is for the application to maintain a user-space data structure that caches the readiness of each of the file descriptors that it is monitoring. 

// so .. cache as (READY) .. until .. recv/send ZERO ...

// EPOLLONESHOT. If this flag is specified in the events mask for a file descriptor, then, once the file descriptor becomes ready and is returned by a call to epoll_wait(), it is disabled from further monitoring (but remains in the interest list). If the application is interested in monitoring file descriptor once more, then it must re-enable the file descriptor using the epoll_ctl(EPOLL_CTL_MOD) operation. 

// A defunct or "zombie" process in Linux occurs when a child process finishes running via fork(), but its parent process does not read its exit status using a wait() system call. The process remains in the table holding its PID until reaped.

ResourceFcgi::~ResourceFcgi()
{
	WsLog::_(LVL_DBG, TGT_RSRC, " (~) ResourceFcgi");
	this->conn_closed();
}

ResourcePiped::~ResourcePiped()
{
	WsLog::_(LVL_DBG, TGT_RSRC, " (~) ResourceCgi");
	WsLog::_(LVL_DBG, TGT_RSRC, "stat: " , this->stat);
	WsLog::_(LVL_DBG, TGT_RSRC, "pid : " , this->pid);
	
	this->conn_closed();
	this->conn = NULL;
	this->wait(WNOHANG);
	if (this->stat == -1 && this->pid)
	{
		// WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_RSRC, "kill");
		kill(this->pid, SIGKILL);
		this->wait(0); // do not set error
	}
}





void	ResourceFcgi::conn_closed(void)
{
	if (this->fcgi)
		this->fcgi->rsrc_closed();
}

void	ResourcePiped::conn_closed(void)
{
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
	// this->conn = NULL;
}

void	ResourceCgi::set_err(int e)
{
	this->error = e;
	if (this->conn)
		this->conn->set_err(e);
}

// NEED HEAD	0
// NEED_BODY	0
// HAVE_RESP	1
// ERROR		2
// DONE			-1

int	ResourceFcgi::status(void)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (2);
	}
		
	if (!this->hed && this->fcgi)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		this->fcgi->mod_evt(EPOLLOUT);	
		return (0); // NEED_HEAD
	}

		// HAVE_SOME_DATA
	if (this->resp.size())
		return (1);
	
	if (this->fcgi == NULL)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (2);
		return (-1);
	}
	// not yet deleted ... 
	// but .. resp still has data

	// STILL RUNNING
	WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	this->fcgi->mod_evt(EPOLLIN);
	return (0); // NEED_DATA
}


int	ResourcePiped::status(void)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (error)");
		return (2);
	}
	if (!this->hed && this->ip)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (no head)");
		this->ip->mod_evt(EPOLLOUT);		
		return (0); // NEED_HEAD
	}

	// HAVE_SOME_DATA
	if (this->resp.size())
		return (1);
	
	if (this->wait(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (exited)");
		if (this->error)
			return (2);
		return (-1);
	}
	// STILL RUNNING
	
	WsLog::_(LVL_DBG, TGT_RSRC_STAT, "stat:  (need data)");

	if (this->op)
		this->op->mod_evt(EPOLLIN);
	return (0); // NEED_DATA
}

// Header Safety: You must send HTTP headers (like Content-Type) before any body text. If an error occurs midway through generating output, buffering lets you discard the partial text and output a clean 500 Internal Server Error page instead of a broken, half-rendered HTML file.

// Content-Length: Holding the output lets you measure the exact byte size of your response so you can send an accurate Content-Length header.

int	ResourceFcgi::wait(int opt)
{
	(void)opt;
	if (this->fcgi)
		return (-1);
	return (0);
}
int	ResourcePiped::wait(int opt)
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
		WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (opt && (this->ip || this->op))
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
	// if (this->stat > 0)
	// 	this->set_err(500);
	this->pid = 0;
	return (this->stat);
}

int	ResourceFcgi::rem(EpollClient *epc)
{
	int err = 0;

	if (epc == this->fcgi)
	{
		this->fcgi = NULL;
		err = 3;
	}
	return (err);
}

int	ResourcePiped::rem(EpollClient *epc)
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
	else 
	
	if (this->ip == NULL && this->op == NULL)
	{
		err = 3;
		this->wait(WNOHANG);
	}	
	return (err);
}

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));		
}

static std::string hedval_str(std::string & str, const char *key)
{
	// std::string	kstr = std::string("\n") + std::string(key);
	std::string	kstr = std::string(key);
	std::string	val("");

	std::string::const_iterator it = std::search(
		str.begin(), str.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == str.end())
        return (val);
    if (it != str.begin() && *(it-1) != '\n')
        return (val);
        
    std::stringstream	line(str.substr(it - str.begin()));
    line >> kstr >> val;
    return (val);

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
	// WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", ostr);	
	this->hed = 1;
	
// REQUIRE (!)
	// Content-Type (?)
	std::string conn_close("Connection: close\r\n");
	ostr.insert(0, conn_close);
	
	std::string stat_hed;
	std::string stat_str = hedval_str(ostr, "Status");
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_str);
	if (stat_str.size())
	{
		// HTTP/1.1 STATUS [Status Message]
		stat_hed = std::string("HTTP/1.0 ") + stat_str + "\r\n";
	}
	else
	{
		stat_hed = std::string("HTTP/1.0 200 OK\r\n");
	}
	ostr.insert(0, stat_hed);
	// WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", this->ostr);	
	return (RSRC_RESP_HEAD);
}

int	ResourceCgi::req_body(void)
{
	Session &sess = conn->sess;
	Request &req  = sess.getRequest();

	if (body.size())
		return (1);
		
	if (!req.hasHeaders())
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "req : (!) hasHeaders");
		return (-1);
	}
	if (!req.hasBody())
	{
		WsLog::_(LVL_DBG, TGT_RSRC, "req : (!) hasBody");
		if (req.isComplete())
			return (-3);
		return (-2);
	}

	Stream * rbody = req.getBody();

    char buf[REQ_READ_SIZ];
    ssize_t err = rbody->readsome(buf, REQ_READ_SIZ);
    WsLog::_(LVL_DBG, TGT_CGI, "SOME: ", err);
	if (err <= 0)
		return (-3);
    body.append(buf, err);
    WsLog::_(LVL_DBG, TGT_CGI, "body: ", body.size());

	return (body.size());
}

int		ResourceCgi::recv_data(char *buf, int siz)
{
	this->resp.append(buf, siz);
	WsLog::_(LVL_DBG, TGT_RSRC, "resp: ", resp.size());

	// WsLog::_(LVL_DBG, TGT_RSRC, "ostr");
	// WsLog::_(LVL_DBG, TGT_RSRC, "****\n", ostr);
	
	return (this->chk_rsp_hed(this->resp));
}


void    ResourceFcgi::push_body(void)
{
	if (this->fcgi)
		this->fcgi->mod_evt(EPOLLOUT);
}

void    ResourcePiped::push_body(void)
{
    if (this->ip)
        this->ip->mod_evt(EPOLLOUT);
}

int	ResourceFcgi::init(Epoll *ep, CgiEnv *cgienv, Connection *conn)
{	
	int err;

	WsLog::_(LVL_DBG, TGT_RSRC, "init:  FCGI");

// WEBSERV : SERVER
	int fd = FcgiConn::make_sock(conn->serv.fcgi_sock.c_str());
	if (fd < 0)
		return (-1);
	
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

int	ResourcePiped::init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn)
{
	int	err;
	WsLog::_(LVL_DBG, TGT_RSRC, "init:  PIPE");
	
	this->pid = _pid;
	
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