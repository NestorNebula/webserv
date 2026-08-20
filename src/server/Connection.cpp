/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/20 21:22:41 by kdonlon          ###   ########.fr       */
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
	if ((this->lact + CONN_TIMEOUT) > now)
		return (false);
		
	// php-fpm : gets this .. 
	// php-cgi : (ip) times out ... but it should not have been active anyway .. 
	WsLog::_(LVL_TMP, TGT_CONN, "TIMEO");
	if (res_cgi)
		WsLog::_(LVL_TMP, TGT_CONN, "cgi state ", res_cgi->done);
	// FWIW : normal (cgi) seems to survive low timeout values better .. 
	if (this->res_cgi)	
		this->res_cgi->conn_closed(); 
	this->set_err(408); // Request Timeout 
	return (true);
}

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
	WsLog::_(LVL_DBG, TGT_CONN, "err:  ", e);
	this->error = e; // why not (?)
	this->sess.setError(e);
	this->mod_evt(EPOLLOUT);
}

ssize_t	Connection::pollin(void)
{
try
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

	sess.log_next();
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
	// Request & req = sess.getRequest();
	// (void)req;
	switch (sess.nextAction()) 
	{
	case Session::DOCGI:
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_DBG, TGT_CONN, "exec: cgi");
			return (0); // send error
		}
		// data has been written to (sess)
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
catch(const std::exception& e)
{
	// not bigaudio.php friendly
	// linked to TIMEOUT (?)
	// set an error (?)
	std::cerr << "POLLIN " << e.what() << '\n';
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
try
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
			return (0);
		case 2: // ERROR
			return (0);
		default:
			break;
		}

		std::string & RESP = res->get_resp();
		
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp");
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);	
		err = this->send(RESP);
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
		std::string & RESP = sess.get_resp();
		if (RESP.size())
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
			// WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp");
			// WsLog::_(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);			
			err = this->send(RESP);
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
	switch (sess.nextAction())
	{
	case Session::KPALIVE:
		this->reset();
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (-1);
	case Session::CLOSE:
		return (-1);
	default:
		break;
	}
	return (err);
}
catch(const std::exception& e)
{
	// not bigaudio.php friendly
	std::cerr << "POLLOUT " << e.what() << '\n';
	this->set_err(404);
}
	return (0);
}

int	Connection::rdhup(void)
{
	WsLog::color(WSL_GREEN);
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
	// ATTN : may need error (?)
	return (-1); 
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

		// pipes.dup_err();

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
