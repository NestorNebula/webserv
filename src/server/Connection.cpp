/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 18:04:14 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"
#include "ResourceFcgi.hpp"
#include "ResourcePiped.hpp"

Connection::Connection	(const Connection & that) : 
	EpollClient(that), 
	sess(that.serv.get_conf()), 
	serv(that.serv), 
	req_cnt(0)
{

}

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	sess(_serv.get_conf()),
	serv(_serv), 
	res_cgi(NULL),
	req_cnt(0)
{
};

Connection::~Connection()
{
	WSLOG(LVL_DBG, TGT_CONN, " (~) Connection ", this->fd);
	WSLOG(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
	try 
	{
		if (this->res_cgi)
		{
			this->res_cgi->conn_closed();
			delete (this->res_cgi);
		}
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, " (~) Connection\n", e.what());
	}
}

// A "connection reset by peer" error (TCP RST packet) means the remote host, firewall, or proxy closed the network connection abruptly. To resolve it, you must identify whether the issue is caused by misconfigured timeouts, aggressive firewalls, stale connection pools, or application code crashes

// the error is the one that shows up after the TCP connection was established. The SYN succeeded, the SYN-ACK succeeded, the ACK succeeded, data flowed, and then the RST appeared.

// socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1))


bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEOUT) > now)
		return (false);
		
	WSCOL(WSL_RED);
	WSLOG(LVL_DBG, TGT_CONN, "TIMEO : conn ", this->get_fd());
	this->set_err(408); // Request Timeout 
	// NEEDS TO CLOSE CONNECTION
	return (true);
}

int	Connection::set_err(int e)
{
	if (e == 0)
		return (-1);
	if (this->error)
	{
		WSLOG(LVL_ERR, TGT_CONN, "err:  already set!");
		WSLOG(LVL_ERR, TGT_CONN, "cur:  ", this->error);
		WSLOG(LVL_ERR, TGT_CONN, "new:  ", e);
		this->mod_evt(EPOLLOUT);
		return (-1);
	}
	WSLOG(LVL_DBG, TGT_CONN, "err:  ", e);
	this->error = e; // why not (?)
	this->sess.setError(e);
	this->mod_evt(EPOLLOUT);
	return (-1);
}

ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	try
	{
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  POLLIN");
		// sess.log_next();
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv");
		err = this->recv();
		if (err < 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_RECV, "recv", err);
			return (err);
		}
		if (err == 0) 
		{
			WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  ZERO");
			this->mod_evt(EPOLLOUT); 
			return (0);
		}
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

		// sess.log_next();
		switch(sess.nextAction())
		{
		case Session::RDSOCK:
		case Session::DOCGI:
			sess.write(this->ibuf, err);
			break;
		case Session::CLOSE:
			return (-1);
		default:
			break;
		}
		
		switch (sess.nextAction()) 
		{
		case Session::DOCGI:
			if (this->exec_cgi() < 0)
			{
				WSLOG(LVL_DBG, TGT_CONN, "exec: cgi");
				return (0); // send error
			}
			this->res_cgi->push_body();
			break;
		case Session::WRSOCK:
			this->req_cnt++;
			this->mod_evt(EPOLLOUT);
			break;
		case Session::RDSOCK:
			break;
		case Session::KPALIVE:
			// return (-1);
			WSLOG(LVL_DBG, TGT_CONN, "keep-alive (ip)");
			return (0);
		case Session::CLOSE:
			return (-1);
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, "ex: pollin\n", e.what());
		this->set_err(404);
	}
	return (0);
}

// ∗ Just remember that, for chunked requests, your server needs to un-chunk them, 
// the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. 
// If no content_length is returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

ssize_t	Connection::pollout(void)
{
	ssize_t	err = 0;
	try
	{
		WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
		// sess.log_next();

		if (sess.nextAction() == Session::DOCGI)
		{
			ResourceCgi *res = this->res_cgi;
			if (res == NULL)
			{
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (NULL)");
				return (-1);
			}
			switch (res->status())
			{
			case RSP_COMPLETE:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (< 0)");
				return (-1);
			case RSP_WAIT_HEAD:
			case RSP_WAIT_BODY:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  no data");
				this->mod_evt(-EPOLLOUT);
				return (0);
			case RSP_ERROR:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (error)");
				return (0);
			default:
				break;
			}

			std::string & RESP = res->get_resp();
			
			WSLOG(LVL_DBG, TGT_CONN_SEND, "send: ", this->get_fd());
			WSLOG(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "resp");
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);	
			err = this->send(RESP);
		}
		else
		{
			switch (sess.nextAction())
			{
			case Session::CLOSE:
				return (-1);
			case Session::WRSOCK:
			default:
				std::string & RESP = sess.get_resp();
				if (RESP.size())
				{
					WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
					WSLOG(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
					// WSLOG(LVL_DBG, TGT_CONN_SEND, "resp");
					// WSLOG(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);			
					err = this->send(RESP);
				}
			}
		}
		
		if (err < 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
			return (err);
		}
		if (err == 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  ZERO");
			return (0);
		}
		WSLOG(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
		
		// sess.log_next();
		switch (sess.nextAction())
		{
		case Session::KPALIVE:
			this->reset();
			// this->mod_evt(-EPOLLOUT);
			// unless we sent back error ...
			WSLOG(LVL_DBG, TGT_CONN, "keep-alive (op) ", this->req_cnt);
			return (0);
			// return (-1);
		case Session::CLOSE:
			return (-1);
		default:
			break;
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, "ex: pollout\n", e.what());
		this->set_err(404);
	}
	return (0);
}

int	Connection::rdhup(void)
{
	WSLOG(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
	return (-1);
}

int	Connection::hup(void)
{
	WSLOG(LVL_DBG, TGT_CONN, "hup!");
	if (this->res_cgi)
		this->res_cgi->conn_closed();
	return (-1);
}

void	Connection::reset(void)
{
	if (this->res_cgi)
	{
		this->res_cgi->conn_closed();
		delete (this->res_cgi);
		this->res_cgi = NULL;
	}

	this->sess.reset();
	this->error = 0;
	this->mod_evt(EPOLLIN);
	this->mod_evt(-EPOLLOUT);
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

void	Connection::cgi_rem(EpollClient *epc)
{
	switch (this->res_cgi->rem(epc))
	{
	case 1: // (ip)
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (ip)   ", this->fd);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (op)   ", this->fd);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (op)   ", this->res_cgi->error);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (DONE) ", this->fd);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	default:
		break;
	}
}

int	Connection::exec_cgi(void)
{
	if (this->res_cgi)
		return (0);
		
	this->req_cnt++;
	
	int			err;

	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WSLOG(LVL_DBG, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (-1);
	}

	if ((cgienv->lang == CGI_PHP) &&
		!this->serv.get_conf().fcgi_sock.empty())
	{
		WSLOG(LVL_DBG, TGT_CGI, "FCGI_SOCK");
		ResourceFcgi * fcgi = new ResourceFcgi;
		err = fcgi->init(this->ep, cgienv, this);
		
		WSLOG(LVL_DBG, TGT_CONN, "php : ", err);
		if (err == 0)
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_CONN, "php :  fcgi");			
			delete (cgienv);
			this->res_cgi = fcgi;
			return (err);
		}
		delete (fcgi);
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CONN, "php :  pipe");
	}

	cgi_pipes	pipes;

	if (pipes.init() < 0)
	{
		delete (cgienv);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes.init");
	}
	pid_t pid = fork();
	if (pid < 0)
	{
		delete (cgienv);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}	
	if (pid == 0)
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

		pipes.dup_err();

		std::string & cwd = cgienv->get("CWD");
		if (cwd.size())
		{
			// WSCOL(WSL_GREEN);
			// WSLOG(LVL_DBG, TGT_CGI, "cwd : ", cwd);
			err = chdir(cwd.c_str());
			if (err < 0)
			{
				WsLog::_errno(LVL_ERR, TGT_CGI_ENV, "chdir");
				pipes.shutdown();
				delete (cgienv);
				delete (this->ep);
				exit(1);				
			}
		}
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);
	
	ResourcePiped * pcgi = new ResourcePiped;
	err = pcgi->init(this->ep, pid, &pipes, this);
	if (err < 0)
	{
		delete (pcgi); // conn : cgi FAIL
		return (this->set_err(503)); // CGI_ERR
	}
	this->res_cgi = pcgi;
	return (err);
}
