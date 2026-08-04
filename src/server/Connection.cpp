/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/04 19:34:46 by kdonlon          ###   ########.fr       */
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
	WsLog::color(2);
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
	if (this->ka)
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


// ResourceCgi (!)
// sess::state
	// pull_data (?)
int		Connection::cgi_done(void)
{
	ResourceCgi *res = this->cgi;

	std::string & OSTR = res->ostr;
	
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  error");
		return (1); // have (error) data to send
	}
	// DELETE_CGI
	if (res == NULL)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  res (NULL)");
		return (-1);
	}
	
	// if (res->stat != -1)
	// {

	// }
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  res (stat) ", res->stat);
	if (!res->hed)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (no head)");
		return (0); // NEED_HEAD
	}
	
	// resource state
		// sent stat
		// sent head
		// sent body
		// unknown (no content-length : wait for cgi-close)
	
// this evaluation should not be here 
		// RESOURCE 
	if (OSTR.size()) 
		return (1); // HAVE_DATA

// OSTR == EMPTY

// attn : end of pollout (sent all)

	if (res->status(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (exited)");
		if (res->error)
		{
			// where SHOULD this have happened (?)
			this->set_err(res->error); 
			return (1);
		}	
		// may still have data to read from cgi (!)
		// return (-1); // DONE
	}

	// BODY FULLY SENT
	if (this->ka && res->tlen == 0)
	{
		// working well 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (tlen) ", res->tlen);
		return (-1);
	}

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  wait for data");
	
	// this->mod_evt(EPOLLIN); // only if more body to send
	if (res->op)
		res->op->mod_evt(EPOLLIN);
	if (res->ip)
		res->ip->mod_evt(EPOLLOUT);
	return (0); // NEED_DATA
}


// ∗ Just remember that, for chunked requests, your server needs to un-chunk them, 
// the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. 
// If no content_length is returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.


#if 0


epoll : evt tgt  : conn
epoll : evt fd   : [144]
epoll : evt typ  : out 
conn  : send:  POLLOUT
conn  : rsnd:  cnt [0]
conn  : done:  res (stat) [-1]
conn  : rsnd:  cgi  data  [1]
conn  : send
conn  : ostr: [8192]
conn  : sent: [8192]
conn  : sent:  all
conn  : rsnd:  cnt [8192]
conn  : rsnd:  cgi tlen [239]
conn  : done:  res (stat) [-1]
rsrc  : exit: [0]
rsrc  : exit:  Success
conn  : done:  cgi (exited)
conn  : rsnd:  cgi  data  [-1]
conn  : rsnd:  conn error [0]
conn  : rsnd:  cgi  error [0]
conn  : rsnd:  conn ka    [1]
conn  : rsnd:  conn keep-alive [5]
rsrc  : (~) ResourceCgi
rsrc  : done: [0]
conn  : -out:  reset


#endif



int		Connection::have_data(void)
{
	// ResourceCgi *res = this->cgi;
	// std::string & OSTR = this->ostr;
	
	// if (this->error)
	// 	return (1);
	// if (OSTR.size())
	// 	return (1);
	// if (res->status(WNOHANG) != -1)
	// 	return (-1);
	// // if (!res->hed)
	// // 	return (0);
		
	// if (res->op)
	// 	res->op->mod_evt(EPOLLIN);
	// if (res->ip)
	// 	res->ip->mod_evt(EPOLLOUT);
	return (0);
}

int		Connection::rsrc_send(int cnt)
{
	int	err;
// yeah -- need something different for BEFORE and AFTER send ... 
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cnt ", cnt);

	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  error ", this->error);
		return (1);
	}
	// if ((cnt == 0) && this->ostr.size())
	// {
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  ostr  ", this->ostr.size());
	// 	return (1);
	// }
	
	// exit(66) -- sometimes .. 200 (0) bytes .. 
	// so .. ostr (?)
	// or .. have head (?)
	// nothing to send 
	
#if 1
	ResourceCgi *res = this->cgi;
	if (res == NULL)
		return (-1);
	if (res->error)
	{
		this->set_err(res->error);
		return (1);
	}
	if (res && res->status(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi (NULL) ", cnt );

		if (res->ostr.size()) // post 
			return (1);

// RES->KA (?)
		if (this->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN, "-out:  rsrc send   (1)");
			if (cnt) // -- like below ..
				this->reset(); // DELETE_CGI
			else
				return (-1);
			// 	this->mod_evt(EPOLLIN);
			return (0);
		}
		return (-1);
	}
#endif
	
	// tlen : should be with .. conn/sess
	if (cnt && this->cgi)
	{
		if (cnt < this->cgi->tlen)
			this->cgi->tlen -= cnt;
		else
			this->cgi->tlen = 0;
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi tlen ", this->cgi->tlen);
	}
	
	err = this->cgi_done();
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  data  ", err);
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn error ", this->error);
		if (this->cgi)
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  error ", this->cgi->error);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn ka    ", this->ka);
	}
	if (err == 0)
	{
		// not ready -- need to wait for data from cgi
		WsLog::_(LVL_DBG, TGT_CONN, "-out:  rsrc send  (2)");
		if (this->cgi->op)
			this->cgi->op->mod_evt(EPOLLIN);
		
		return (0);
	}
	if (err < 0)
	{
// RES->KA (?)
		if (this->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn keep-alive");
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn req   ", this->req_cnt);
			// did we already send an error on this (fd)
			if (cnt)
			{
				this->reset(); // DELETE_CGI
			}
			return (0);
		}
		else
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  delete (?)");
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

// have_data : don't care about cgi state 

int		Connection::send_error(void)
{
	int	err;
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
// RES->KA (?)
	if (this->error == 408 && this->ka)
	{
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  t/o (ka)");
		this->error = 0;

		this->mod_evt(EPOLLIN); // only if more body to send
		if (this->cgi)
		{
			if (this->cgi->op)
				this->cgi->op->mod_evt(EPOLLIN);
			if (this->cgi->ip)
				this->cgi->ip->mod_evt(EPOLLOUT);
			return (0);
		}
		return (-1);
	}
	err = this->send(this->estr); 
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
	if (err < 0)
		return (-1);
	if (this->estr.size())
		return (err);
	if ((this->error != 408) && this->ka) 
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "err :  keep-alive");
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "req : ", this->req_cnt); 
		this->reset(); // DELETE_CGI
		return (0);
	}
	return (-1);
}

ssize_t	Connection::pollout(void)
{
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
	
	ssize_t	err = 0;
	
// what is the REAL QUESTION I want to be asking here ... 
	err = this->rsrc_send(0);
	// err = this->have_data();
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
		this->mod_evt(-EPOLLOUT);
		return (err);
	}	
	
	if (this->error)
		return (this->send_error());
	
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
	
	this->rsrc_send(err);
	return (err);
}

int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
	
	if (this->cgi)
		this->cgi->conn_closed();
	if (this->ka)
	{
		this->ka = 0;
		return (0);
	}
	return (-1); // (hm)
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
	WsLog::_(LVL_DBG, TGT_CONN, "-out:  reset");
	this->mod_evt(-EPOLLOUT);
	this->mod_evt(EPOLLIN);
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
void	Connection::cgi_rem(CgiPipe *epc)
{
	switch (this->cgi->rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (ip)   ", this->fd);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (op)   ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err : (op)   ", this->cgi->error);
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (DONE) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err : (op)   ", this->cgi->error);
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
