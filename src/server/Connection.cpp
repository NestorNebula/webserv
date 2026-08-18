/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/18 17:43:18 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"

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
	WsLog::_(LVL_DBG, TGT_CONN, " (~) Connection ", this->fd);
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
	if (this->res_cgi)
	{
		this->res_cgi->conn_closed();
		delete (this->res_cgi); // (~) Connection
	}
};

bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEOUT) < now) // server (?)
	{
		this->set_err(408);
		return (true);
	}
	return (false);
}

// WEBSERV : ERROR
void	Connection::set_err(int e)
{
	if (e == 0)
		return;
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN, "err:  already set!");
		WsLog::_(LVL_DBG, TGT_CONN, "cur:  ", this->error);
		WsLog::_(LVL_DBG, TGT_CONN, "new:  ", e);
		this->mod_evt(EPOLLOUT);
		return;
	}
	// this->error = e;
	WsLog::_(LVL_DBG, TGT_CONN, "err:  ", e);
	this->sess.setError(e);
	this->mod_evt(EPOLLOUT);
}

ssize_t	Connection::pollin(void)
{
	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv:  POLLIN");
	sess.log_next();
	
	ssize_t	err;

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv", err);
		return (err);
	}
	if (err == 0) 
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv:  ZERO");
		this->mod_evt(EPOLLOUT); 
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

// WEBSERV : SESSION (write)
	sess.log_next();
	// if (sess.nextAction() == Session::RDSOCK)
	switch(sess.nextAction())
	{
	case Session::RDSOCK:
	case Session::DOCGI:
		sess.write(this->ibuf, err);
		break;
	default:
		break;
	}
	switch (sess.nextAction()) 
	{
		case Session::DOCGI:
			if (this->exec_cgi() < 0)
			{
				WsLog::_(LVL_DBG, TGT_CONN, "exec: cgi");
				this->set_err(404); // CGI_ERR
				// this->set_err(404); // siege-friendly
				return (0); // send error
			}
			// req_cnt
			this->res_cgi->push_body();
			break;
		case Session::WRSOCK:
			this->req_cnt++;
			this->mod_evt(EPOLLOUT);
			break;
		case Session::RDSOCK:
			break;
		case Session::KPALIVE:
			return (-1);
		case Session::CLOSE:
			return (-1);
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
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
	
	ssize_t	err = 0;
	
	sess.log_next();
	if (sess.nextAction() == Session::DOCGI)
	{
		ResourceCgi *res = this->res_cgi;
		if (res == NULL)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "res : (NULL)");
			return (-1);
		}
		err = res->status();
		if (err < 0)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "res : (< 0)");
			if (res->resp.size() == 0)
				return (-1);
		}
		switch (err)
		{
		case 0:
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
			this->mod_evt(-EPOLLOUT);
			return (err);
		case 2: // set_err
			return (0);
		default:
			break;
		}

		std::string & OSTR = res->get_resp();
		
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr: " , OSTR.size());
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr");
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "****\n", OSTR);	
		err = this->send(OSTR);
	}
	else
	{
		if (sess.nextAction() == Session::CLOSE)
		{
			return (-1);
		}
		if (sess.nextAction() != Session::WRSOCK)
		{

		}
		std::string & OSTR = sess.get_resp();
		if (OSTR.size())
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr: " , OSTR.size());
			// WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr");
			// WsLog::_(LVL_DBG, TGT_CONN_SEND, "****\n", OSTR);			
			err = this->send(OSTR);
		}
		else
		{
			// done (?)
		}
	}
	
	if (err < 0)
	{
		WsLog::_errno(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	// if (OSTR.size())
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", OSTR.size());
	// else
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent:  all");
	
	sess.log_next();
	if (sess.nextAction() == Session::KPALIVE)
	{
		this->reset();
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (-1);
	}
	return (err);
}

int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	// check res status (?)
	this->mod_evt(EPOLLOUT);
	return (-1); // may need error (?)
	return (0);
}

int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	if (this->res_cgi == NULL)
		return (-1);
	this->res_cgi->conn_closed();
	return (-1);
}


void	Connection::reset(void)
{
	if (this->res_cgi)
	{
		this->res_cgi->conn_closed();
		delete (this->res_cgi); // conn : reset 
		this->res_cgi = NULL;
	}
	
// WEBSERV : SESSION (keep-alive)
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

// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read.
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it


// SESSION / REQUEST (CgiPipe::pollout)
// CGI input may need to know :
	// (1)	: body data has been received by the Connection
	//		  and needs to be written to the (stdin) of the CGI
	// (0)	: no body data is currently available
	//		  BUT .. more needs to be received to complete the request
	// (-1) : there is no more body data to write to the CGI

// WEBSERV : REQUEST (body)
int	Connection::req_body_status(void)
{
	int	err = -1; // DEMO (!) this->sess.req.body_stat();

	if (err == 1) // body.size()
		return (1);
	if (err == 0) // not done
		return (0); 
		
	this->mod_evt(EPOLLOUT); // seems wrong 		
	return (-1);
}

// called on ~CgiPipe()
void	Connection::cgi_rem(EpollClient *epc)
{
	switch (this->res_cgi->rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (ip)   ", this->fd);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (op)   ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (op)   ", this->res_cgi->error);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (DONE) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
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
		WsLog::_(LVL_DBG, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (-1);
	}
	
	if (cgienv->lang == CGI_PHP) // PHP_FPM fcgi_sock
	{
		ResourceFcgi * fcgi = new ResourceFcgi;
		err = fcgi->init(this->ep, cgienv, this);
		
		WsLog::_(LVL_DBG, TGT_CONN, "php : ", err);
		if (err == 0)
		{
			WsLog::color(WSL_GREEN);
			WsLog::_(LVL_DBG, TGT_CONN, "php :  fcgi");			
			delete (cgienv);
			this->res_cgi = fcgi;
			return (err);
		}
		delete (fcgi);
		WsLog::color(WSL_YELLOW);
		WsLog::_(LVL_DBG, TGT_CONN, "php :  pipe");
	}

	cgi_pipes	pipes;

	if (pipes.init() < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes.init");

	// WsLog::pwd();
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
// char cwd[PATH_MAX];
		const char **envp = cgienv->gen();

		// WsLog::color(WSL_RED);
		// WsLog::_(LVL_DBG, TGT_CGI, "exec: ", cgienv->args[0]);
		// WsLog::color(WSL_RED);
		// WsLog::_(LVL_DBG, TGT_CGI, "path: ", cgienv->args[1]);

		pipes.dup_err();

		std::string & cwd = cgienv->get("CWD");
		// REQUIRE (!)
		if (cwd.size())
		{
			// WsLog::color(WSL_GREEN);
			// WsLog::_(LVL_DBG, TGT_CGI, "cwd : ", cwd);
			err = chdir(cwd.c_str());
			if (err < 0)
				return (WsLog::_errno(LVL_ERR, TGT_CGI_ENV, "chdir"));
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
		this->set_err(503); // CGI_ERR
		return (err);
	}
	this->res_cgi = pcgi;
	return (err);
}


// we can get this when we ctrl-c a siege

// not fatal .. but we should be able to handle it better

// epoll : cli mod  : [352]
// epoll : epoll_ctl: mod 
// error : No such file or directory


	// THE FULL SEQUENCE
// epoll : evt tgt  : conn
// epoll : evt fd   : [8]
// epoll : evt typ  : in rdhup 
// conn  : recv:  POLLIN
// conn  : recv
// epc   : read: [168]
// conn  : recv: [168]
// conn  : next:  RDSOCK
// conn  : next:  DOCGI
// conn  : next: docgi

// rsrc  : init:  PIPE
// epoll : cli add  : cgi
// epoll : cli add  : cgi

	// wow .. CGI has not yet even gotten started
// conn  : RDHUP
// epoll : cli mod  : conn
// epoll : cli rem  : conn
// epoll : cli del  : conn
// conn  :  (~) Connection [8]
// conn  : req cnt: [0]
// rsrc  : conn-closed : ip
// rsrc  : conn-closed : op

// rsrc  :  (~) ResourceCgi
// rsrc  : stat: [-1]
// rsrc  : pid : [17211]
// rsrc  : conn-closed : ip
// rsrc  : conn-closed : op
// set ip/op to NULL ... 
// set CONN to NULL
// rsrc  : pid : [17211]
// rsrc  : xit : [-1]
// rsrc  : stat: [-1]
// rsrc  : kill
// rsrc  : pid : [17211]
// rsrc  : xit : [-1]
// rsrc  : stat: [-1]
// rsrc  : wait: [17211]
// rsrc  : stat: [9]
// rsrc  : sig : [9]
// rsrc  : sig : Killed
// conn  : err : [616]
// conn  : fd  : (conn) [8]
// epoll : cli mod  : conn
// epoll : cli mod  : [8]
// epoll : epoll_ctl: mod 
// error : No such file or directory
// epc   :  (~) EpollClient
