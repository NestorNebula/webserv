/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/30 21:26:34 by kdonlon          ###   ########.fr       */
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
		this->set_err(408);
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
		// cgi_data => head => php (404) => 504
		WsLog::_(LVL_DBG, TGT_CONN, "err:  already set!");
		WsLog::_(LVL_DBG, TGT_CONN, "cur:  ", this->error);
		WsLog::_(LVL_DBG, TGT_CONN, "new:  ", e);
		this->mod_evt(EPOLLOUT);
		return;
	}

	WsLog::_(LVL_DBG, TGT_CONN, "err : ", e);
	WsLog::_(LVL_DBG, TGT_CONN, "ostr: ", ostr.size());
	
	// ATTN : some errors (500) are not siege-friendly
// SESSION - get_op_data .. 
	std::string ebody("Error Data\r\n");
	
	this->error = e;
	this->estr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n");

	if (this->ka)
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
			// not 100% sure here 
		// this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT); 
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	int req_state = sess.write(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);

// SESSION::error
	// have_head
	// init_rsrc
	if (this->cgi == NULL)
	{
		this->cgi = new ResourceCgi;
		this->ka = sess.req.ka;
	}
	if (this->cgi->pid == 0)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec: cgi");
			// this->set_err(503); // CONN - new ResourceCgi failed
			this->set_err(404);
			// this->mod_evt(-EPOLLIN); // uncertain
			return (0); // send error (!)
		}
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
	
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  error");
		return (1); // have data to send
	}


	if (res == NULL)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  res (NULL)");
		return (-1);
	}
#if 0 // VERY VERY PROBLEMATIC -- working well without this 
	if (res->status(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (exited)");
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (stat) ", res->stat);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (err ) ", res->error);		
		if (res->error)
		{
			this->set_err(res->error); // where SHOULD this have happened (?)
			return (1);
		}			
		return (-1); // DONE
	}
#endif



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
	
	// Q: move into ResourceCgi (?)
	// works well -- not the right place for it 
	if (ostr.size()) 
		return (1); // HAVE_DATA

	// (ostr.size() == 0)

// post-send-checks
// more like "done" tests here 
	// // we just checked this in pollout
	if (res->status(WNOHANG) != -1)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (exited)");
		if (res->error)
		{
			this->set_err(res->error); // where SHOULD this have happened (?)
			return (1);
		}	
		return (-1); // DONE
	}


// KEEP_ALIVE
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  cgi (tlen) ", res->tlen);

	// tlen -- even if not (ka) ? 
	if (this->ka && res->tlen == 0)
	{
		// working well 
		return (-1); // DONE -- but test for (ka) should decide to kill
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
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "done:  wait for data");
	
// TIMEOUT (?)
	// this->mod_evt(-EPOLLOUT); // => pollout
	if (res->op)
		res->op->mod_evt(EPOLLIN);
		
	// this->mod_evt(EPOLLIN); // only if more body to send
	if (res->ip)
		res->ip->mod_evt(EPOLLOUT);
	return (0); // NEED_DATA
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
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  error ", this->error);
		return (1);
	}
	if (this->cgi == NULL)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi (NULL)");
		if (this->ostr.size())
			return (1);

#if 1
		if (this->ka)
		{
			// need to reset here (?)
			// timeout (?)
			WsLog::_(LVL_DBG, TGT_CONN, "-out: rsrc send   (1)");
			// this->mod_evt(-EPOLLOUT);
			// RESET
				// not quite
			// should have been called when (cgi) was deleted
			// fucks with pollin
			this->reset();
			return (0);
		}
#endif
// and : we LOSE fucking cgi tlen .. when we delete the cgi
// fuck ; stderr seems to play a role here .. wtf
		// return (1); // still data in the pipe to read (?)
		return (-1);
	}
	if (cnt && this->cgi) // ->ka)
	{
		if (cnt < this->cgi->tlen)
			this->cgi->tlen -= cnt;
		else
		{
			this->cgi->tlen = 0;
			// this->reset();
			// return (0);
		}
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi tlen ", this->cgi->tlen);
	}
	
	err = this->cgi_done();
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  data  ", err);
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn error ", this->error);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  error ", this->cgi->error);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn ka    ", this->ka);
	}
	if (err == 0)
	{
		// not ready -- need to wait for data from cgi
		WsLog::_(LVL_DBG, TGT_CONN, "-out:  rsrc send  (2)");
		// if (this->cgi && this->cgi->op)
		// 	this->cgi->op->mod_evt(EPOLLIN);
		// this->mod_evt(-(EPOLLOUT));
		return (0);
	}
	if (err < 0)
	{
		if (this->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  conn keep-alive ", this->req_cnt);
			// ugh : may KILL prematurely
			// BEFORE we SEND
			// maybe nothing sent 
			if (cnt)
				this->reset();
			return (0);
		}
		else
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "rsnd:  cgi  delete (?)");
			// test elsewhere (?)
			// delete (this->cgi);
			// this->cgi = NULL; // wtf
		}
		return (-1);
	}
	return (err);
}
// rsrc_send
	// error
	// cgi == NULL
	// cgi_done
		// error
		// status (NOHANG)
		// hed
		// ostr
		// status (NOHANG)
		// ka / tlen
		

// fuck : did the cgi finishing .. CLOSE the socket .. across the FORK (?)
ssize_t	Connection::pollout(void)
{
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
	ssize_t	err = 0;
	

// sometimes, bigimage.php does not send all data .... 
// wanna be : real careful .. about when (cgi) is deleted / done
// mod_evt(0)
// actually deleted / removed ... with connection (?)
// feels fucky to have that happening in the Epoll::loop()

// PROOF (of problem) : keep-alive sucks

// bigimage.php : byte count is DIFFERENT under (siege) ... 

	// err = this->sess.pull_data(this->ostr);
#if 1
	err = this->rsrc_send(0);
	if (err <= 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  no data    ", err);
		// this->mod_evt(0); // get clean (rdhup)
		this->mod_evt(-EPOLLOUT);
		// fucking (cgi == NULL) + (ka) = reset => POLLIN
		return (err);
	}	
#endif
// SESSION
// kd : integration
	//  How should we "switch" from ResourceCgi to ResourceError (send file ...)
// not on some mp3 waiting .. 
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
		if (this->error == 408 && this->ka)
		{
			// this->error = 0;
			// return (0);
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
		// we get here .. if we did not check rsrc_send at head of function
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
	// we do NOT want to kill right away (?)
	
	// return (err);
	// we DO need to check (tlen) stuff ...
	// 
	this->rsrc_send(err);
	return (err);
	// return (this->rsrc_send(err));
}

// FUCK : why would re-directing stderr make this work (?)
// but : NOT when logging NO ERRORS
// but : no log at all .. same problem
// some sort of .. slow-down (?)
int	Connection::rdhup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "RDHUP");
	this->ka = 0;
	this->mod_evt(EPOLLOUT);
	
	// fuck : current REQUEST
	// if (this->cgi == NULL)
	// 	return (0);

	// or : kill on "out rdhup"
#if 1
	if (this->cgi && this->cgi->ip)
	{
		this->cgi->ip->rsrc_closed(); // rsrc_closed
		this->cgi->ip->mod_evt(EPOLLIN);
	}
	if (this->cgi && this->cgi->op)
	{
		this->cgi->op->rsrc_closed();
		this->cgi->op->mod_evt(EPOLLOUT);
	}
#endif
	// (!)
	return (-1);
}


// epoll : evt tgt  : cgi
// epoll : evt fd   : [13]
// epoll : evt typ  : hup 
// epoll : cli rem  : cgi
// epoll : cli del  : cgi
// rsrc  : done: [0]
// conn  : rem : cgi (DONE) [7]
// conn  : rem : err (op)   [0]
// rsrc  : (~) ResourceCgi
// rsrc  : reset
// epoll : cli mod  : conn



int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	if (this->cgi == NULL)
		return (-1);
		
	if (this->cgi->ip)
	{
		this->cgi->ip->rsrc_closed(); // rsrc_closed
		this->cgi->ip->mod_evt(EPOLLIN);
	}
	if (this->cgi->op)
	{
		this->cgi->op->rsrc_closed();
		this->cgi->op->mod_evt(EPOLLOUT);
	}
	return (-1);
}


void	Connection::reset(void)
{
	this->sess.reset();
	if (this->cgi)
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

	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", ostr);
	
	if (res->hed)
	{
		this->mod_evt(EPOLLOUT);
		return (0);
	}
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
		
// rsrc::parse_head
	WsLog::_(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	res->hed = 1;
	res->hlen = pos + 4;
	
// std::string stat;
// std::string head;
// std::string body; // (ostr)
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

	std::string conn_close("Connection: close\r\n");
	std::string conn_keep("Connection: keep-alive\r\n");
	
	std::string conn_val = hedval_str(this->ostr, "Content-Length");
	if (conn_val.size())
	{
		res->clen = atoi(conn_val.c_str());
		res->tlen = res->hlen + res->clen;
		
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
		this->ka = 0;
		this->ostr.insert(0, conn_close);
	}
	
	std::string stat_200("HTTP/1.0 200 OK\r\n");
	this->ostr.insert(0, stat_200);
	res->tlen += stat_200.size();
	
	// WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", ostr);	
	return (0);
}


// called in ~CgiPipe()
// may trigger cgi->status(0)
// ResourceCgi (!)
void	Connection::cgi_rem(CgiPipe *epc)
{
	switch (this->cgi->rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (ip)   ", this->fd);
		if (this->cgi->error)
			this->set_err(this->cgi->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (op)   ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err : (op)   ", this->cgi->error);
		if (this->cgi->error)
			this->set_err(this->cgi->error);
		this->mod_evt(EPOLLOUT);
		break;
	case 3: // (done)
		WsLog::_(LVL_DBG, TGT_CONN, "rem cgi : (DONE) ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN, "rem err : (op)   ", this->cgi->error);
		if (this->cgi->error)
			this->set_err(this->cgi->error);	
		delete(this->cgi);
		this->cgi = NULL;
		this->mod_evt(EPOLLOUT);
		break;
	default:
		break;
	}
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

		// signal(SIGINT, SIG_DFL);
		// WsLog : CGI_ERR
		pipes.dup_err();
		// delete(this->ep);
		// this->ep->dupx();
		// any (fd) CLOSED here ... 
		// will be removed from the epoll set of the parent  

		// int sf = dup(this->fd);
		// (void)sf;
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);

	// this->cgi = new ResourceCgi;

	err = this->cgi->init(this->ep, pid, &pipes, this);
	// if (err < 0)
	// 	this->cgi->reset();
	return (err);
}
