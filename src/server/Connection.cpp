/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 15:27:07 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"

Connection::Connection (Epoll *_ep, int _fd, br_Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
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

	WsLog::_(LVL_DBG, TGT_CONN, "err : ", e);
	WsLog::_(LVL_DBG, TGT_CONN, "fd  : (conn) ", this->fd);
	
	std::string ebody("Error Data\r\n");
	
	this->error = e;
	this->estr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n");

	this->estr += std::string("Connection: close\r\n");
	this->estr += std::string("Content-Length: ") + num_2_str(ebody.size()) + std::string("\r\n");
	this->estr += std::string("\r\n");
	this->estr += ebody;

	// WsLog::_(LVL_DBG, TGT_CONN, "err:\n", this->estr);
	this->mod_evt(EPOLLOUT);
}

ssize_t	Connection::pollin(void)
{
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
	int req_state = sess.write(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);

	if (this->res_cgi == NULL)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_DBG, TGT_CONN, "exec: cgi");
			// this->set_err(503); // CGI_ERR
			this->set_err(404); // siege-friendly
			return (0); // send error
		}
	}
	
// SESSION
	this->res_cgi->push_body();
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

// WEBSERV : ERROR
	if (this->error)
		return (this->send_error());

// WEBSERV : RESOURCE (cgi)
	ResourceCgi *res = this->res_cgi;
	if (res == NULL)
	{
		return (-1);
	}
	err = res->status();
	if (err < 0)
	{
		if (res->resp.size() == 0)
			return (-1);
	}	
// WEBSERV : ERROR
	if (this->error)
		return (this->send_error());
		
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
		this->mod_evt(-EPOLLOUT);
		return (err);
	}	
	
// WEBSERV : RESOURCE (response)
	std::string & OSTR = res->get_resp();

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr: " , OSTR.size());
	// WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr");
	// WsLog::_(LVL_DBG, TGT_CONN_SEND, "****\n", OSTR);
	err = this->send(OSTR);
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
	if (OSTR.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", OSTR.size());
	else
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent:  all");
	
	return (err);
}

// WEBSERV : ERROR 
int		Connection::send_error(void)
{
	int	err;
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);

	err = this->send(this->estr); 
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
	if (err < 0)
		return (-1);
	if (this->estr.size())
		return (err);
	return (-1);
}

int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
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
	}
	this->res_cgi = NULL;
	
// WEBSERV : SESSION (keep-alive)
	this->sess.reset();
	
	this->estr.clear();
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
// kd : CGI input may need to know :
	// (1)	: body data has been received by the Connection
	//		  and needs to be written to the (stdin) of the CGI
	// (0)	: no body data is currently available
	//		  BUT .. more needs to be received to complete the request
	// (-1) : there is no more body data to write to the CGI

int	Connection::req_body_status(void)
{
// WEBSERV : REQUEST (body)
	int	err = this->sess.req.body_stat();

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
			WsLog::_(LVL_DBG, TGT_CONN, "php : fcgi");			
			delete (cgienv);
			this->res_cgi = fcgi;
			return (err);
		}
		delete (fcgi);
		WsLog::color(WSL_YELLOW);
		WsLog::_(LVL_DBG, TGT_CONN, "php : pipe");
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
		pipes.dup_err();

		const char **envp = cgienv->gen();

		std::string & cwd = cgienv->get("CWD");
		if (cwd.size())
		{
			// WsLog::_(LVL_DBG, TGT_CGI, "cwd  : ", cwd);
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
