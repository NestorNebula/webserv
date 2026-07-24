/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 18:00:52 by kdonlon          ###   ########.fr       */
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
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection"); // , this->fd);
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
		this->set_err(408); // CONN : timed out
// siege : bigaudio .. some hang -- wrong state (?)
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
	}

	WsLog::_(LVL_DBG, TGT_CONN, "err : ", e);
	
	// ATTN : some errors (500) are not siege-friendly
// SESSION - get_op_data .. 
	this->error = e;
	this->estr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n\r\nError Data\r\n");

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
#if 0 // unclear if this is necessary
		// cgi_state() ? 
		if (this->cgi.status(WNOHANG) > 0)
		{
			// ATTN : seems like we'd want to send an error here ...
			// assume it's been set (?)
			// keep-alive (?)
			// or .. let pollout take care of it 
			this->mod_evt(EPOLLOUT);
			// return (-1);
		}
#endif
		this->mod_evt(-EPOLLIN);
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
		this->cgi = new ResourceCgi;
	if (this->cgi->pid == 0)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec: cgi");
			this->set_err(503); // CONN - new ResourceCgi failed
			// this->mod_evt(-EPOLLIN);
			this->mod_evt(EPOLLOUT);
			return (0);
		}
	}
// SESSION
	// body_data
	this->cgi->push_data();
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
	
	// rsrc::chk_err
	if (this->error == 0)
	{
		if (this->cgi->status(WNOHANG) > 0)
			this->set_err(this->cgi->error);
	}
		
// SESSION
// kd : integration
	//  How should we "switch" from ResourceCgi to ResourceError (send file ...)
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
		err = this->send(this->estr); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
		if (err < 0)
			return (-1);
		if (this->estr.size())
			return (err);
		return (-1);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");

// SESSION
// kd : integration
	//  the std::string ostr is flushed to the client,
	//  but may be filled by the CGI
	
// seems like we need to "check done" up here, too
// part of sess::pull_data / fill_ostr

// SESSION : fetch from resource .. if necessary
	// into fixed (ostr) here (?)
	// sess->fetch(str & ostr)
		// if (ostr.size())
			// return it 
		// else .. fill from .. stream
	// if (ostr.size() == 0)
	// rsrc::fill_ostr(ostr); // or : this sets some (state)
// I wanted cgi to fill this DIRECTLY
	if (this->ostr.size() == 0)
	{
		err = this->sess.pull_data(this->ostr);
		// STATE ! 
	}

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
		// cgi.status() ? 
		this->mod_evt(-EPOLLOUT);
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	if (ostr.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent:  all");
		// this->mod_evt(-EPOLLOUT); 
	}
	
		// part of sess::pull_data()
	if (this->cgi->ka)
	{
		if (err < this->cgi->tlen)
			this->cgi->tlen -= err;
		else
			this->cgi->tlen = 0;
	}
	// sess_check_done();
#if 1 // BOTH (ugly)
	err = this->cgi_done(); // cgi_check();
	// (-1) : done : close / keep-alive
	// (0)  : in progress, no data
	// (1)  : have data to write
	if (err == 0)
		return (0);
	if (err < 0)
	{
// SESSION / REQUEST - move back to pollout
		if (this->cgi->ka)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  keep-alive ", this->req_cnt);
			this->reset();
			return (0);
		}
		return (-1);		
	}
#endif
	
	
	return (err); // (!) bytes written
}

// rdhup : may want to close (cgi.ip)
int	Connection::hup(void)
{
	WsLog::_(LVL_DBG, TGT_CONN, "hup!");
	return (-1);
}


void	Connection::reset(void)
{
	this->sess.reset();
	this->cgi->reset(); // (Q) : with pointer 
	this->ostr.clear();
	this->estr.clear();
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
		
	this->mod_evt(-EPOLLIN); 
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
int	Connection::cgi_data(const char *buf, ssize_t siz)
{
	this->ostr.append(buf, siz);

	this->mod_evt(EPOLLOUT);
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", ostr);
	
	// OR 
	// (cgi)
	// str	stat;
	// str	head;
	// str	body;

// RESOURCE::push_data()
	// request (ostr) from Resource (like the others)
	// track how much of (clen) sent
	// so we know -- WITHIN THE RESOURCE
	// when we're done
	// the problem (for keep-alive)
		// "done" was detected by cgi closing
	if (this->cgi->hed == 0)
	{
		size_t	pos = ostr.find("\r\n\r\n");
		if (pos == std::string::npos)
			return (0);
		WsLog::_(LVL_DBG, TGT_CGI_DATA, "HEAD");
		this->cgi->hed = 1;
		this->cgi->hlen = pos + 4;
// REQUEST
// kd : 

// WsLog::_(LVL_DBG, TGT_CGI_RECV, "head:");
// WsLog::_(LVL_DBG, TGT_CGI_RECV, "****\n", ostr);
		std::string val;
		
		val = hedval_str(this->ostr, "Content-Length");
		if (val.size())
		{
			this->cgi->clen = atoi(val.c_str());
			this->cgi->tlen = this->cgi->hlen + this->cgi->clen;
// need this for KEEP_ALIVE
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "hlen: ", this->cgi->hlen);
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "clen: ", this->cgi->clen);
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "tlen: ", this->cgi->tlen);
			WsLog::_(LVL_DBG, TGT_CGI_HEAD, "OSTR:\n", ostr);			
		}
		val = hedval_str(ostr, "Connection");
		if (val.size())
		{
			std::transform(val.begin(), val.end(), val.begin(), ::tolower);
			if (val == std::string ("keep-alive"))
				this->cgi->ka = 1;
		}

		std::string head_conn("Connection: close\r\n");
		if (this->cgi->ka == 0)
			this->ostr.insert(0, head_conn);
		std::string stat("HTTP/1.1 200 OK\r\n");
		
		val = hedval_str(ostr, "Status");
		// wtf
		WsLog::_(LVL_DBG, TGT_CGI_HEAD, "stat: ", val);
		if (!val.size())
		{
			this->ostr.insert(0, stat);
			// tlen may not have been set (!)
			if (this->cgi->ka)
				this->cgi->tlen += stat.size();
			return (0);
		}
		int http_stat = atoi(val.c_str());
		if (http_stat != 200)
			this->set_err(http_stat); // CGI : Status header
		else
		{
			this->ostr.insert(0, stat);
			if (this->cgi->ka)
				this->cgi->tlen += stat.size();
		}
		// I .. may need to ADD keep-alive here (?)
	}
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
		// this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (op) ", this->fd);
		this->mod_evt(EPOLLOUT); // ugh
		break;
	default:
		break;
	}
}

// ResourceCgi (!)
// sess::state
	// pull_data (?)
int		Connection::cgi_done(void)
{
	if (!this->cgi->hed)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: cgi (no head)");
		this->mod_evt(-EPOLLOUT); 
		return (0);
	}
	
	// resource state
		// sent stat
		// sent head
		// sent body
		// unknown (no content-length : wait for cgi-close)
	
	if (ostr.size()) // has_data
		return (1);

// more like "done" tests here 
	// we just checked this in pollout
	if (this->cgi->status(WNOHANG) != -1)
	{

		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: cgi (exited)");
		 // CGI is DONE	
		 return (-1);
	}


// KEEP_ALIVE
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: cgi (tlen) ", this->cgi->tlen);
	if (this->cgi->ka && this->cgi->tlen == 0)
	{
		return (-1);
	}
// fine .. 
// but have not RESET 
	// CGI is still active ... 

// PROBLEM : we get more keep-alive data BEFORE our cgi finishes cleanly

	// could know DONE here .. from content-length
// need to track : cgi: clen && slen
	// BUT : may have sent ALL on keep-alive
	// AND : cgi is taking its time closing ...
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: wait for data");
	
// TIMEOUT (?)
	this->mod_evt(-EPOLLOUT);
	if (this->cgi->op)
		this->cgi->op->mod_evt(EPOLLIN);
		
	this->mod_evt(EPOLLIN); // only if more body to send
	if (this->cgi->ip)
		this->cgi->ip->mod_evt(EPOLLOUT);
	return (0);
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
		pipes.dup_err();
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
