/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/06 12:06:55 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	cgi(NULL),
	serv(_serv), 
	req_cnt(0),
	ka(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, " (~) Connection ", this->fd);
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
	if (this->cgi)
		delete (this->cgi);
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

// SESSION
void	Connection::set_err(int e)
{
	if (e == 0)
		return;
// epoll : evt tgt  : conn
// epoll : evt fd   : [118]
// epoll : evt typ  : out 
// conn  : send:  POLLOUT
// conn  : rsnd:  cnt [0]
// conn  : send
// conn  : ostr: [0]
// conn  : send:  ZERO

	// return; // major stalls .. no data to send 
	// e = 200; // => cgi NULL (408 not caught);
	// if (e != 408)
	// 	e = 200;
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
	WsLog::_(LVL_DBG, TGT_CONN, "ka  : (conn) ", this->ka);
	if (this->cgi)
		WsLog::_(LVL_DBG, TGT_CONN, "ka  : (res)  ", this->cgi->ka);
	
	// ATTN : some errors (500) are not siege-friendly
// SESSION - get_op_data .. 
	std::string ebody("Error Data\r\n");
	
	this->error = e;
	this->estr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n");

// res : head .. may have turned it OFF .. 
// but we don't care on ERROR 
	if (this->ka) // error
		this->estr += std::string("Connection: keep-alive\r\n");
	else
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

// SESSION
	int req_state = sess.write(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);

	this->mod_evt(EPOLLOUT); 
	if (this->cgi == NULL)
	{
		// this->cgi = new ResourceCgi;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec: cgi");
			// this->set_err(503);
			this->set_err(404); // siege-friendly
			return (0); // send error
		}
// (ka) : from REQUEST
		this->ka = sess.req.ka;
		this->cgi->ka = sess.req.ka;
		this->req_cnt++;
	}
	
// SESSION
	this->cgi->push_body();
	return (err);
}

// ∗ Just remember that, for chunked requests, your server needs to un-chunk them, 
// the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. 
// If no content_length is returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

int		Connection::rsrc_send(int cnt)
{
	int	err;

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cnt ", cnt);
	// if (this->error)
	// {
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  error ", this->error);
	// 	return (1);
	// }
	
	ResourceCgi *res = this->cgi;
	if (res == NULL)
	{
		// WsLog::color(WSL_GREEN);

// still have have data/something to do (!)

// FUCKING STATES

		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi (NULL) ", cnt );
		return (-1);
	}
	
	if (res->error)
	{
		this->set_err(res->error);
		return (1);
	}

#if 1 // why not just (status)
	// without : can STALL 
	if (res->wait(WNOHANG) != -1)
	{
		// DONE (1)
		// WsLog::color(WSL_RED);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi (wait) ", cnt );

		if (res->ostr.size()) // post 
			return (1);
// RES->KA (?)
		if (this->ka && res->ka)
		{
			// WsLog::color(WSL_GREEN);
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "keep-alive (1)");
			WsLog::_(LVL_DBG, TGT_CONN, "-out:  rsrc send   (1)");
		
// this does not seem right
			if (cnt) // -- like below ..
				this->reset(); // DELETE_CGI
// does not seem right	
			else
				return (-1);
// we were getting a double-pollin on keep-alive (?)
			// 	this->mod_evt(EPOLLIN);
			return (0);
		}
		return (-1);
	}
#endif
	
	err = res->status(); // MAY SET ERROR
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  data  ", err);
	if (err < 0)
	{
// RES->KA (?)
		if (this->ka && res->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn keep-alive");
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn req   ", this->req_cnt);
			// did we already send an error on this (fd)
			// should be in POLLOUT
			if (cnt)
			{
				// WsLog::color(WSL_RED);
				WsLog::_(LVL_DBG, TGT_CONN_SEND, "keep-alive (2)");
				this->reset(); // DELETE_CGI
			}
			// WsLog::color(WSL_YELLOW);
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "keep-alive (3)");
			return (0);
		}
		return (-1);
	}
	return (err);
}


// need to be real careful .. about when (cgi) is deleted / done

// THE ANSWER : 
	// a VERY clear idea .. of what we need to know when
	// cgi->state
	// check_state()
// cgi .. might be "done" .. but .. it's when the .. pipes are closed (?) that matters
// (Q) : when do we DELETE the resource 

ssize_t	Connection::pollout(void)
{
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
	
	ssize_t	err = 0;

	if (this->error)
		return (this->send_error());

	err = this->rsrc_send(0);
	if (err <= 0)
	{
		// what if we detect an error in here (?)
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
		this->mod_evt(-EPOLLOUT);
		return (err);
	}	
	// if (this->error)
	// 	return (this->send_error());
	

	// MOVE rsrc_send to res->get_data()
		
	
	// res->get_ostr .. 
	ResourceCgi *res = this->cgi; // ASSUME non-NULL
	// err = this->sess.pull_data();
	std::string & OSTR = res->ostr;

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr: " , OSTR.size());
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
	
	res->consumed(err);
	this->rsrc_send(err);
	return (err);
}

int		Connection::send_error(void)
{
	int	err;
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
#if 1
	if (this->error == 408 && this->ka)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  t/o (ka)");
		this->error = 0;
		this->mod_evt(EPOLLIN); // so .. this not set .. 
		if (this->cgi)
		{
			// this->cgi->conn_closed(); // BAD!
			if (this->cgi->op)
				this->cgi->op->mod_evt(EPOLLIN);
			if (this->cgi->ip)
				this->cgi->ip->mod_evt(EPOLLOUT);
			return (0);
		}
		return (0);
		// return (-1);
	}
#endif
	err = this->send(this->estr); 
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
	if (err < 0)
		return (-1);
	if (this->estr.size())
		return (err);
		
	if ((this->error != 408) && this->ka) 
	{
		// WsLog::color(WSL_GREEN);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "err :  keep-alive");
		// WsLog::color(WSL_GREEN);


		// this->mod_evt(EPOLLIN);
		if (this->cgi)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "err :  cgi != NULL");
		}
		else
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "err :  cgi == NULL");
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "req : ", this->req_cnt); 
		this->reset(); // DELETE_CGI
		return (0);
	}
	return (-1);
}

int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
	
	if (this->cgi)
	{
		this->cgi->ka = 0;
		// this->cgi->conn_closed();
		// delete (this->cgi);
		// this->cgi = NULL;
	}
	if (this->ka)
	{
		this->ka = 0;
		return (0);
	}
	// return (-1); // (this feels dangerous)
	return (0);
}

int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	if (this->cgi == NULL)
		return (-1);
	this->cgi->conn_closed();
	return (-1);
}


void	Connection::reset(void)
{
	this->sess.reset();
	if (this->cgi)
		delete (this->cgi);
	this->cgi = NULL;
	
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

// Sesssion : should have pushed data to resource 
// ResourceCgi (!)
int	Connection::req_body_status(void)
{
	int	err = this->sess.req.body_stat(); // SESSION / REQUEST

	if (err == 1) // body.size()
		return (1);
	if (err == 0) // not done
		return (0); 
		
	this->mod_evt(EPOLLOUT);		
	return (-1);
}

// called on ~CgiPipe()
// which could .. directly .. 
// except .. sometimes we set conn = NULL
void	Connection::cgi_rem(CgiPipe *epc)
{
	switch (this->cgi->rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (ip)   ", this->fd);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (op)   ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (op)   ", this->cgi->error);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi  : (DONE) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err  : (op)   ", this->cgi->error);
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
		WsLog::_(LVL_ERR, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (-1);
	}


	// need new EpollClient .. 
	// which is constructed with an EXISTING (fd)
	// so .. resource .. would create the sock 
	// this->cgi = new ResourceFastCgi(ep, fd, cgienv, this)

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

		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);
	
	this->cgi = new ResourceCgi;
	err = this->cgi->init(this->ep, pid, &pipes, this);
	if (err < 0)
	{
		delete (this->cgi);
		this->cgi = NULL;
	}
	return (err);
}
