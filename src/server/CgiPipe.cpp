/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/20 13:59:46 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiPipe.hpp"
#include "Connection.hpp"
#include "Server.hpp"


static	void fd_close(int *fd)
{
	if (*fd == -1)
		return;
	close(*fd);
	*fd = -1;
}

cgi_pipes::cgi_pipes (void)
{
	p1[0] = -1;
	p1[1] = -1;
	p2[0] = -1;
	p2[1] = -1;
	dnfd  = -1;
}

cgi_pipes::~cgi_pipes()
{
	this->shutdown();
}

int	cgi_pipes::init(void)
{
	if (pipe(p1) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	if (pipe(p2) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "pipe"));
	}
	return (0);
}

int	cgi_pipes::dup_io(void)
{
	if (p1[0] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p1[0], STDIN_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stdin)"));
	}
	if (p2[1] == -1)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup_io"));
	}
	if (dup2(p2[1], STDOUT_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stdout)"));
	}
	return (0);		
}

int	cgi_pipes::dup_err(void)
{
	dnfd = open("/dev/null", O_WRONLY);
	if (dnfd < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "open (/dev/null)"));
	}
	if (dup2(dnfd, STDERR_FILENO) < 0)
	{
		this->shutdown();
		return (WsLog::_errno(LVL_ERR, TGT_CGI, "dup2 (stderr)"));
	}
	fd_close(&dnfd);
	return (0);
}

void	cgi_pipes::shutdown(void)
{
	fd_close(p1);
	fd_close(p1 + 1);
	fd_close(p2);
	fd_close(p2 + 1);
	fd_close(&dnfd);
}




CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn)
{
}
	
CgiPipe::~CgiPipe()
{
	WsLog::_(LVL_DBG, TGT_CGI, "(~) Cgi");
	if (this->conn)
		this->conn->cgi_rem(this);
}

bool	CgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + EPC_TIMEOUT) < now) // server (?)
	{
		if (this->conn)
			this->conn->set_err(408); // script timed out .. 
		// kill here (?)
		return (true);
	}
	return (false);
}

ssize_t	CgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
		
// SESSION : check_status()

	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	if (err < 0)
	{
		this->conn->set_err(500);
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv: err");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv:  ZERO");
		return (-1);
	}
// SESSION
	if (this->conn->cgi_data(this->ibuf, err) < 0)
		return (-1);
	
	return (err);
}

// The server is in no way obligated to send end-of-file after the script reads CONTENT_LENGTH bytes. 
ssize_t	CgiPipe::pollout(void)
{
	if (this->conn == NULL)
		return (-1);
// SESSION : check_status()
	if (this->conn->cgi_status(WNOHANG) > 0)
	{
		return (-1);
	}	
	
	ssize_t	err;
	
// SESSION
	err = this->conn->req_body_status();
	if (err < 0) // body is complete AND fully flushed
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body: complete");
		// close fd .. should trigger script, right (?)
		return (-1);
	}
	if (err == 0) // body is not complete, but no data currently available
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body: waiting");
		this->mod_evt(0);
		return (0);
	}

// SESSION : Request::body
	std::string & body = this->conn->sess.req.get_body();
	
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", body.size());
	err = this->send(body);
	if (err < 0)
	{
		this->conn->set_err(500);
		WsLog::_(LVL_ERR, TGT_CGI_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "send:  ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "sent: ", err);
	return (0);
}

int		CgiPipe::hup(void)
{
	if (this->conn == NULL)
		return (-1);
	this->conn->mod_evt(EPOLLOUT);
	return (-1);
}

void	CgiPipe::conn_closed(void)
{ 
	this->conn = NULL;
}




ResourceCgi::~ResourceCgi()
{
	WsLog::_(LVL_DBG, TGT_RSRC, "(~) ResourceCgi");
	
	this->reset();
}

void	ResourceCgi::reset(void)
{
	if (this->ip || this->op) // pid, stat (?)
	{
		if (this->stat == -1)
		{
			kill(this->pid, SIGKILL);
			this->status(0);
		}
	}
	if (this->ip)
	{
		this->ip->conn_closed();
		// this->ip->mod_evt(EPOLLIN);
	}
	if (this->op)
	{
		this->op->conn_closed();
		// this->op->mod_evt(EPOLLOUT);
	}
	this->pid  = 0;
	this->ip   = NULL;
	this->op   = NULL;
	this->stat = -1;
	this->hed  = 0;
}

int	ResourceCgi::status(int opt)
{
	WsLog::_(LVL_DBG, TGT_RSRC, "pid : ", this->pid);
	WsLog::_(LVL_DBG, TGT_RSRC, "xit : ", this->xit);
	WsLog::_(LVL_DBG, TGT_RSRC, "stat: ", this->stat);
	if (this->stat != -1)
	{
		WsLog::_(LVL_INFO, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	if (this->pid == 0)
	{
		WsLog::_(LVL_INFO, TGT_RSRC_INFO, "done: ", this->stat);
		return (this->stat);
	}
	
	int err = waitpid(this->pid, &this->stat, opt);
	
	WsLog::_(LVL_DBG, TGT_RSRC, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_RSRC, "stat: ", stat);

	if (err == 0)
		return (this->stat); // WNOHANG : no change .. (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
		WsLog::_(LVL_INFO, TGT_RSRC_INFO, "exit: ", xit);
		char *serr = std::strerror(xit);
		WsLog::_(LVL_ERR, TGT_RSRC, "exit: ", std::string(serr)); // valgrind
	}
	else if (WIFSIGNALED(stat))
	{
		this->sig = WTERMSIG(stat);
		WsLog::_(LVL_INFO, TGT_RSRC_INFO, "sig : ", sig);
		WsLog::_(LVL_ERR, TGT_RSRC, "sig : ", strsignal(sig));
	}
	else
	{
		WsLog::_(LVL_INFO, TGT_RSRC_INFO, "STAT: ", stat);
	}
	this->pid = 0;
	return (this->stat);
}

void	ResourceCgi::rem(CgiPipe *epc)
{
	if (epc == this->ip)
	{
		this->ip = NULL;
		if (this->op)
			this->op->mod_evt(EPOLLIN);
	}
	else if (epc == this->op)
		this->op = NULL;
	if (this->ip == NULL && this->op == NULL)
		this->status(0);
}

