/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 13:21:50 by kdonlon          ###   ########.fr       */
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




CgiPipe::CgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourceCgi * _rsrc) : 
	EpollClient(_ep, EPC_CGI, _fd), 
	conn(_conn),
	rsrc(_rsrc)
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
		if (this->rsrc)
			this->rsrc->set_err(408); // CGI : timed out .. 
		// kill here (?)
		return (true);
	}
	return (false);
}

ssize_t	CgiPipe::pollin(void)
{
	// rsrc::
	// conn should have told (rsrc) something 
	// when it closed
	if (this->conn == NULL)
		return (-1);

	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv: ", err);
	if (err < 0)
	{
// rsrc::set_err() 
		this->rsrc->set_err(501); // CGI : read failed
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "recv: err");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_RECV, "recv:  ZERO");
		return (-1);
	}
	// rsrc::push_data() => rsrc::ostr
	if (this->conn->cgi_data(this->ibuf, err) < 0)
		return (-1);
	
	return (err);
}

// The server is in no way obligated to send end-of-file after the script reads CONTENT_LENGTH bytes. 
ssize_t	CgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
	// rsrc::status
	if (this->rsrc->status(WNOHANG) >= 0)
		return (-1);
	
	
// SESSION / REQUEST
// kd : CGI input may need to know :
	// (1)	: body data has been received by the Connection
	//		  and needs to be written to the (stdin) of the CGI
	// (0)	: no body data is currently available
	//		  BUT .. more needs to be received to complete the request
	// (-1) : there is no more body data to write to the CGI
	
// hasBody()
// getBody()
// isComplete()
	// rsrc:: should have been filled from sess::write

	// sess::
	err = this->conn->req_body_status();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body:  complete");
		return (-1);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CGI_SEND, "body:  waiting");
		this->mod_evt(0);
		return (0);
	}

// SESSION / REQUEST
// kd : Connection currently stores the request body in a std::string
	// EpollClient::send() erases the sent bytes from the head of the string
	
	// sess::write
	// should have pushed non-header data (body)
	// to rsrc::idata ...
	// rsrc::get_body
	// which should have been properly filled
	// by sess::write
	std::string & body = this->conn->sess.req.get_body();
	WsLog::_(LVL_DBG, TGT_CGI_SEND, "send: ", body.size());
	err = this->send(body);
	if (err < 0)
	{
		this->rsrc->set_err(502); // CGI : write failed
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
	return (-1);
}

void	CgiPipe::rsrc_closed(void)
{ 
	this->conn = NULL;
	this->rsrc = NULL;
}




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

