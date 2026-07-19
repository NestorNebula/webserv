/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/19 12:59:32 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"


ResourceCgi::~ResourceCgi()
{
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "(~) ResourceCgi");
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
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "pid : ", this->pid);
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "xit : ", this->xit);
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "stat: ", this->stat);
	if (this->stat != -1)
	{
		return (this->stat);
	}
	if (this->pid == 0)
	{
		return (this->stat);
	}
	
	int err = waitpid(this->pid, &this->stat, opt);
	
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "stat: ", stat);

	// hm : (0) for (0)
	if (err == 0)
		return (this->stat); // WNOHANG : no change .. (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_CGI_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		this->xit = WEXITSTATUS(stat);
		WsLog::_(LVL_DBG, TGT_CGI_RSRC, "exit: ", xit);
		// (2) : No such file or directory
		WsLog::_(LVL_ERR, TGT_CGI_RSRC, "exit: ", strerror(xit));
	}
	else if (WIFSIGNALED(stat))
	{
		this->sig = WTERMSIG(stat);
		WsLog::_(LVL_DBG, TGT_CGI_RSRC, "sig : ", sig);
	}
	this->pid = 0;
	return (this->stat);
}


void	ResourceCgi::rem(CgiPipe *epc)
{
	if (epc == this->ip)
		this->ip = NULL;
	else if (epc == this->op)
		this->op = NULL;
	if (this->ip == NULL && this->op == NULL)
		this->status(0);
}



Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	serv(_serv), 
	req_cnt(0)
{
};


Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
};

bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + EPC_TIMEOUT) < now)
	{
		this->set_err(408);
		this->mod_evt(EPOLLOUT);
		return (true);
	}
	return (false);
}

void	Connection::set_err(int e)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN, "err:  already set!");
	}
	// ATTN : some errors (500) are not siege-friendly
	this->error = e;
	std::string err_str = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n\r\nError Data\r\n");

	this->ostr.insert(0, err_str);
	this->mod_evt(EPOLLOUT);
}

ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
		return (err);
	}
	if (err == 0) 
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv:  ZERO");
#if 1
		if (this->cgi.status(WNOHANG) > 0)
			return (-1); // (?)
#else
			// sets error .. 
			// but we may already be sending (?)
		if (this->cgi_status(WNOHANG) > 0)
			this->mod_evt(EPOLLOUT);
#endif
		this->mod_evt(-EPOLLIN);
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

// SESSION : write()
	int req_state = sess.push_data(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);
		
	if (this->cgi.pid == 0)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			this->set_err(500);
			// this->mod_evt(-EPOLLIN);
			this->mod_evt(EPOLLOUT);
			return (0);
		}
	}
	if (this->cgi.ip)
	{
		// cig.input_available()
		this->cgi.ip->mod_evt(EPOLLOUT);
		// this->mod_evt(EPOLLOUT);
	}
	return (err);
}


// ∗ Just remember that, for chunked requests, your server needs to un-chunk them, 
// the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. 
// If no content_length is returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

ssize_t	Connection::pollout(void)
{
	ssize_t	err = 0;
	
	if (this->error == 0)
		this->cgi_status(WNOHANG);
		
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: error");
		err = this->send(this->ostr); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "err : ", err);
		if (err < 0)
			return (-1);
		if (this->ostr.size())
			return (err);
		return (-1);
	}
	
	if (!this->cgi.hed)
	{
		this->mod_evt(-EPOLLOUT); 
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");

	
	if (ostr.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");

		if (this->cgi.status(WNOHANG) != -1)
		{
#if 0 // KEEP_ALIVE // -- would REQUIRE CONTENT-LENGTH from (cgi)

			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: keep-alive");
			this->sess.req.clear();
			this->cgi.reset();
			this->mod_evt(-EPOLLOUT);
			this->mod_evt(EPOLLIN);
			return (0);
#else
			return (-1);		
#endif			
		}
		// if (this->cgi.pid)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: wait for data");
			this->mod_evt(-EPOLLOUT);
			return (0);
		}
		return (-1);
	}
	


	
	err = this->send(ostr);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  ZERO");
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	if (ostr.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: all");
		// this->mod_evt(-EPOLLOUT); 
	}
	
	return (err); // (!) bytes written
}

int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	return (-1);
}



void	Connection::set_addr(struct sockaddr_in *a)
{
	this->addr = *a; 
	this->astr = addr_2_str(a);
}

std::string		&Connection::get_addr(void) 
{
	return (this->astr);
}


// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read.
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
	
// SESSION
int	Connection::req_body_status(void)
{
	if (this->sess.req.get_body().size())
		return (1);
	// if (body-fully-received) // ASSUMED
	{
		this->mod_evt(-EPOLLIN); // keep-alive (?)
		this->mod_evt(EPOLLOUT);		
		return (-1);
	}
	return (0);
}

int	Connection::cgi_data(const char *buf, ssize_t siz)
{
	this->ostr.append(buf, siz);
	this->mod_evt(EPOLLOUT);
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", ostr);
	
	if (this->cgi.hed == 0)
	{
		size_t	pos = ostr.find("\r\n\r\n");
		if (pos == std::string::npos)
			return (0);
			
		this->cgi.hed = 1;
// WsLog::_(LVL_DBG, TGT_CGI_RECV, "head:");
// WsLog::_(LVL_DBG, TGT_CGI_RECV, "****\n", ostr);
		pos = ostr.find("Status");
		if (pos == std::string::npos)
		{
// size_t	pos = ostr.find("\r\n\r\n");
// WsLog::_(LVL_ERR, TGT_CGI_DATA, "ostr");
// WsLog::_(LVL_ERR, TGT_CGI_DATA, "****\n", ostr.substr(0, pos + 4));
			
			this->ostr.insert(0, std::string("HTTP/1.1 200 OK\r\n"));
// pos = ostr.find("\r\n\r\n");
// WsLog::_(LVL_ERR, TGT_CGI_DATA, "****\n", ostr.substr(0, pos + 4));			
			return (0);
		}

	    std::stringstream	line(ostr.substr(pos));
		std::string key;
		std::string val;
		line >> key >> val;
		WsLog::_(LVL_ERR, TGT_CGI_RECV, "stat: ", val);

		int http_stat = atoi(val.c_str());
		if (http_stat != 200)
			this->set_err(http_stat);
		else
			this->ostr.insert(0, std::string("HTTP/1.1 200 OK\r\n"));
	}
	return (0);
}


void	Connection::cgi_rem(CgiPipe *epc)
{
	this->cgi.rem(epc);
}

int		Connection::cgi_status(int opt)
{ 
	int	err;

	err = this->cgi.status(opt);
	if (err > 0)
	{
		if (this->cgi.xit == 2)
			this->set_err(404);
		else
			this->set_err(500);
	}
	return (err); 
}

int	Connection::exec_cgi(void)
{
	int			err;
	cgi_pipes	pipes;

	if (pipes.init() < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes.init");

	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (-1);
	}
		
	cgi.pid = fork();
	if (cgi.pid < 0)
	{
		delete (cgienv);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}	
	if (cgi.pid == 0)
	{
		err = pipes.dup_io();
		if (err < 0)
		{
			pipes.shutdown();
			delete (cgienv);
			delete (this->ep);
			exit(1);
		}

		const char **envp = cgienv->gen();

		signal(SIGINT, SIG_DFL);
		pipes.dup_err();
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);
	
// fcntl .. F_SETFD .. O_CLOEXEC
	WsLog::_(LVL_DBG, TGT_CONN, "exec cgi");

	// ResourceCgi::init(conn, pipes, ep)
	int cgifd_ip = dup(pipes.p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	
	int cgifd_op = dup(pipes.p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	}	

	this->cgi.ip = new CgiPipe(this->ep, cgifd_ip, this);
	err = this->cgi.ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->cgi.op = new CgiPipe(this->ep, cgifd_op, this);
	err = this->cgi.op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	return (err);
}
