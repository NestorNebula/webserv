/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/28 20:50:44 by kdonlon          ###   ########.fr       */
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
	req_cnt(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection ", this->fd);
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
	if ((this->lact + EPC_TIMEOUT) < now) // server (?)
	{
		this->set_err(408); // CONN : timed out .. on input (?) output (?) cgi (?)
		// KA : shutdown
		this->mod_evt(EPOLLOUT);
		return (true);
	}
	return (false);
}

// SESSION
// kd : How should this be handled when in "cgi" mode 
void	Connection::set_err(int e)
{
	if (e == 0)
		return;
	if (this->error)
	{
		WsLog::_(LVL_ERR, TGT_CONN, "err:  already set!");
		WsLog::_(LVL_ERR, TGT_CONN, "cur:  ", this->error);
		WsLog::_(LVL_ERR, TGT_CONN, "new:  ", e);
		return;
	}

	WsLog::_(LVL_DBG, TGT_CONN, "err : ", e);
	WsLog::_(LVL_DBG, TGT_CONN, "ostr: ", ostr.size());
	
	// if (ostr.size())
	// 	return;
		
	// ATTN : some errors (500) are not siege-friendly
// SESSION - get_op_data .. 
// cgi - no head -- does not get to this (!)

	std::string ebody("Error Data\r\n");
	
	this->error = e;
	this->estr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n");

	// fuck : error set .. before head parsed (?)
	// Q: always close on error (?)
	if (this->cgi && this->cgi->ka)
		this->estr += std::string("Connection: keep-alive\r\n");
	else
		this->estr += std::string("Connection: close\r\n");
	this->estr += std::string("Content-Length: ") + num_2_str(ebody.size()) + std::string("\r\n");
	this->estr += std::string("\r\n");
	this->estr += ebody;

	WsLog::_(LVL_DBG, TGT_CONN, "err:\n", this->estr);
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
#if 0
		// fuck .. xit is (0)
		// but stat is (-1)
		// because WE RESET 
		if (this->cgi && this->cgi->status(WNOHANG) >= 0)
		{
			// ATTN : seems like we'd want to send an error here ...
			// assume it's been set (?)
			// keep-alive (?)
			// or .. let pollout take care of it 
			this->mod_evt(-EPOLLIN);
			// return (-1);
		}
#endif
// epoll : evt typ  : in out rdhup 
// conn  : recv
// conn  : recv:  ZERO
// conn  : (~) Connection
// conn  : req cnt: [4]

		this->mod_evt(-EPOLLIN);
		// this->mod_evt(EPOLLOUT);
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	int req_state = sess.write(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);
	this->ka = sess.req.ka; // ugly
// SESSION::error
	// have_head
	// init_rsrc
	if (this->cgi == NULL)
		this->cgi = new ResourceCgi;
	if (this->cgi->pid == 0)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec: cgi");
			this->set_err(503); // CONN - new ResourceCgi failed
			// this->mod_evt(-EPOLLIN);
			// this->mod_evt(EPOLLOUT); wait for data from CGI
			return (0);
		}
		// does cgi need to know this (?)
		this->cgi->ka = this->sess.req.ka;
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

	if (this->error)
	{
		return (1);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsrc:  error ", err);
	}
	if (this->cgi == NULL)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsrc:  cgi (NULL)");
		if (this->ka)
		{
			// need to reset here (?)
			// timeout (?)
			WsLog::_(LVL_DBG, TGT_CONN, "-out: rsrc_send  (1)");
			this->mod_evt(-EPOLLOUT);
// conn  : -out: rsrc_send (1)
// epc   : mod_evt  : CUR in out 
// epc   : mod_evt  : NEW out rdhup err hup 


			return (0);
		}
		return (-1);
	}
	if (cnt && this->cgi->ka)
	{
		if (cnt < this->cgi->tlen)
			this->cgi->tlen -= cnt;
		else
		{
			this->cgi->tlen = 0;
			// this->reset();
			// return (0);
		}
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsrc:  cgi tlen ", this->cgi->tlen);
	}
	err = this->cgi_done();
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi  done  ", err);
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  conn error ", this->error);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi  error ", this->cgi->error);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  conn ka    ", this->ka);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi  ka    ", this->cgi->ka);
	}
	if (err == 0)
	{
		// not ready -- need to wait for data from cgi
		WsLog::_(LVL_DBG, TGT_CONN, "-out:  rsrc_send  (2)");
// conn  : -out:  rsrc_send  (2)
// epc   : mod_evt  : CUR in out rdhup 
// epc   : mod_evt  : NEW out rdhup err hup 

		this->mod_evt(-(EPOLLOUT));
		return (0);
	}
	if (err < 0)
	{
		if (this->cgi->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi  keep-alive ", this->req_cnt);
			// ugh : may KILL prematurely
			// BEFORE we SEND
			// maybe nothing sent 
			if (cnt)
				this->reset();
			return (0);
		}
		else
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi  delete");
			delete (this->cgi);
			this->cgi = NULL; // wtf
		}
		return (-1);		
	}
	return (err);
}

// fuck : did the cgi finishing .. CLOSE the socket .. across the FORK (?)
ssize_t	Connection::pollout(void)
{

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
	ssize_t	err = 0;
	
	// err = this->sess.pull_data(this->ostr);
	err = this->rsrc_send(0);
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
		return (err);
	}	
// SESSION
// kd : integration
	//  How should we "switch" from ResourceCgi to ResourceError (send file ...)
	if (this->error)
	{
		if (this->error == 408 && this->ka) // cgi && this->cgi->ka)
			return (-1);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
		err = this->send(this->estr); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
		if (err < 0)
			return (-1);
		if (this->estr.size())
			return (err);
		// if (this->error != 408 && this->cgi->ka) // fuck - got reset (1)
		if ((this->error != 408) && this->ka) 
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "err :  keep-alive ", this->req_cnt);
			this->reset();
			return (0);
		}
		return (-1);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "ostr: " , this->ostr.size());
	err = this->send(ostr);
	if (err < 0)
	{
		WsLog::_errno(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  ZERO");
		// cgi.status() 
		this->mod_evt(-EPOLLOUT);
		WsLog::_(LVL_DBG, TGT_CONN, "-out:  send");
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	if (ostr.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent:  all");
	}
	return (this->rsrc_send(err));
}

// FUCK : why would re-directing stderr make this work (?)
// but : NOT when logging NO ERRORS
// but : no log at all .. same problem
// some sort of .. slow-down (?)
int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "rdhup");
	this->mod_evt(EPOLLOUT);
	
	// fuck : current REQUEST
	// if (this->cgi == NULL)
	// 	return (0);

	// or : kill on "out rdhup"
#if 1
	if (this->cgi && this->cgi->ip)
	{
		this->cgi->ip->rsrc_closed(); // rsrc_closed
		// this->ip->mod_evt(EPOLLIN);
	}
	if (this->cgi && this->cgi->op)
	{
		this->cgi->op->rsrc_closed();
		// this->op->mod_evt(EPOLLOUT);
	}
#endif
	// if (this->sess.req.ka)
	// {
	// 	delete (this->cgi);
	// 	this->cgi = NULL;rsrc_send
	// 	return (-1);
	// }
	return (-1);
	if (this->ka)
		return (-1);
	return (0);
}

int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	if (this->cgi == NULL)
		return (-1);
		
	if (this->cgi->ip)
	{
		this->cgi->ip->rsrc_closed(); // rsrc_closed
		// this->ip->mod_evt(EPOLLIN);
	}
	if (this->cgi->op)
	{
		this->cgi->op->rsrc_closed();
		// this->op->mod_evt(EPOLLOUT);
	}
	return (-1);
}


void	Connection::reset(void)
{
	this->sess.reset();
	delete (this->cgi);
	this->cgi = NULL;
	this->ostr.clear();
	this->estr.clear();
	this->error = 0;
	WsLog::_(LVL_DBG, TGT_CONN, "-out: reset");
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
		
	// this->mod_evt(-EPOLLIN); 
	this->mod_evt(EPOLLOUT);		
	return (-1);
}

// (CgiPipe::pollin)
// so .. the CgiPipe .. should really
// just push to its RESOURCE
// which will track what we need 
	// stat
	// head
	// body
// and answer the request for data accordingly
// ResourceCgi (!)

// rsrc::push_data
int	Connection::cgi_data(const char *buf, ssize_t siz)
{
	ResourceCgi *res = this->cgi;
	
	this->ostr.append(buf, siz);

	// rsrc::conn
	this->mod_evt(EPOLLOUT); // only if hed ?

	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", ostr);
	
	// OR 
	// (cgi)
	// str	stat;
	// str	head;
	// str	body;

	if (res->hed)
		return (0);

// RESOURCE::push_data()
	// request (ostr) from Resource (like the others)
	// track how much of (clen) sent
	// so we know -- WITHIN THE RESOURCE
	// when we're done
	// the problem (for keep-alive)
		// "done" was detected by cgi closing

	size_t	pos = ostr.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (0);
		
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	res->hed = 1;
	res->hlen = pos + 4;
	
	
// need to check STATUS first .. 
// may set_error .. which HAS CONTENT_LENGTH .. and CAN DO KEEP_ALIVE

		
	std::string stat("HTTP/1.1 200 OK\r\n");
	
	std::string stat_val = hedval_str(ostr, "Status");
	
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_val);

	if (stat_val.size())
	{
		int http_stat = atoi(stat_val.c_str());
		if (http_stat != 200)
		{
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "STAT: ", http_stat);
			this->set_err(http_stat); // CGI : Status header
			return (0);
		}		
	}

#if 0 // AFTER
	if (!val.size())
	{
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "NO STATUS\n", this->ostr);


		this->ostr.insert(0, stat);
// ATTN : may need to force Connection: close
		// tlen may not have been set (!)
		if (this->ka)
			res->tlen += stat.size();
		// return (0);
// fuck : prepending .. 			
	}
	else
	{
		int http_stat = atoi(val.c_str());
		if (http_stat != 200)
		{
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "STAT: ", http_stat);
			this->set_err(http_stat); // CGI : Status header
			return (0);
		}
			// strange double-insert
		// this->ostr.insert(0, stat);
		// if (this->ka)
		// 	res->tlen += stat.size();
	}
#endif
	std::string conn_close("Connection: close\r\n");
	std::string conn_keep("Connection: keep-alive\r\n");
	
	std::string conn_val = hedval_str(this->ostr, "Content-Length");
	if (conn_val.size())
	{
		res->clen = atoi(conn_val.c_str());
		res->tlen = res->hlen + res->clen;
// need this for KEEP_ALIVE
// If you're going to hold the connection open for more than one message 
// you will need some way for the receiver to determine where the message-boundary lies.
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "hlen: ", res->hlen);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "clen: ", res->clen);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "tlen: ", res->tlen);
		
		if (this->ka)
		{
			this->ostr.insert(0, conn_keep);
			res->tlen += conn_keep.size();
		}
		else
		{
			this->ostr.insert(0, conn_close);
			res->tlen += conn_close.size();
		}
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "conn: error ", this->error);
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "cgi : error ", res->error);
		// fuck -- no error yet
		// problem : when php returns (404) in header 

		res->ka = 0; // ATTN : error (?)
		this->ka = 0;
		this->ostr.insert(0, conn_close);
	}
	
	this->ostr.insert(0, stat);
// ATTN : may need to force Connection: close
	// tlen may not have been set (!)
	if (this->ka)
		res->tlen += stat.size();
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", ostr);	
	return (0);
}


// called in ~CgiPipe()
// may trigger cgi->status(0)
// ResourceCgi (!)
void	Connection::cgi_rem(CgiPipe *epc)
{
	// or : rsrc (cgi) holds pointer to (conn)
	switch (this->cgi->rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (ip) ", this->fd);
		if (this->cgi->error)
			this->set_err(this->cgi->error);
		this->mod_evt(-EPOLLIN);
		// this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (op) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem : err (op) ", this->cgi->error);
		if (this->cgi->error)
			this->set_err(this->cgi->error);
		// not seeing this (!)
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (DONE) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem : err (op) ", this->cgi->error);
		if (this->cgi->error)
			this->set_err(this->cgi->error);		
		delete(this->cgi);
		this->cgi = NULL;
		this->mod_evt(EPOLLOUT);
		break;
	default:
		break;
	}
	// if both are null 
}

// ResourceCgi (!)
// sess::state
	// pull_data (?)
int		Connection::cgi_done(void)
{
	ResourceCgi *res = this->cgi;
	
	if (this->error)
		return (1); // have data to send
		
	// if (res->status(WNOHANG) != -1)
	// {
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi (exited)");
	// 	return (-1); // DONE
	// }
	if (!res->hed)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi (no head)");
		// may still have more to send (1)
		if (this->error > 0)
		{
			
			// res->ka = 0;
			// this->ka = 0;
			
			// this->req_cnt > 1 .... 
			// may still have to send error page ...
			// on failed (cgi) startup 
			// do not want to send an error ... 
			// to a zombie keep-alive
			// if (this->error == 408)
			// 	return (-1);
			return (1); // SEND ERROR
		}
		// return (this->error);
		return (0); // NEED_HEAD
	}
	
	// resource state
		// sent stat
		// sent head
		// sent body
		// unknown (no content-length : wait for cgi-close)
	
	if (ostr.size()) 
		return (1); // HAVE_DATA

	// (ostr.size() == 0)

// post-send-checks
// more like "done" tests here 
	// // we just checked this in pollout
	if (res->status(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi (exited)");
		return (-1); // DONE
	}


// KEEP_ALIVE
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  cgi (tlen) ", res->tlen);
	if (res->ka && res->tlen == 0)
	{
		return (-1); // DONE
	}
	// NEED_DATA
// fine .. 
// but have not RESET 
	// CGI is still active ... 

// PROBLEM : we get more keep-alive data BEFORE our cgi finishes cleanly

	// could know DONE here .. from content-length
// need to track : cgi: clen && slen
	// BUT : may have sent ALL on keep-alive
	// AND : cgi is taking its time closing ...
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  wait for data");
	
// TIMEOUT (?)
	// this->mod_evt(-EPOLLOUT); // => pollout
	if (res->op)
		res->op->mod_evt(EPOLLIN);
		
	this->mod_evt(EPOLLIN); // only if more body to send
	if (res->ip)
		res->ip->mod_evt(EPOLLOUT);
	return (0); // NEED_DATA
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

		signal(SIGINT, SIG_DFL);
		// WsLog : CGI_ERR
		// pipes.dup_err();
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);

	// this->cgi = new ResourceCgi;

	err = this->cgi->init(this->ep, pid, &pipes, this);
	if (err < 0)
		this->cgi->reset();
	return (err);
}
