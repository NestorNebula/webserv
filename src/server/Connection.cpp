/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/22 11:34:27 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	serv(_serv), 
	req_cnt(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection"); // , this->fd);
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
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
	if (this->error)
	{
		WsLog::_(LVL_ERR, TGT_CONN, "err:  already set!");
	}

	WsLog::_(LVL_DBG, TGT_CONN, "err : ", e);
	WsLog::_(LVL_DBG, TGT_CONN, "ostr: ", this->ostr.size());
	
	// ATTN : some errors (500) are not siege-friendly
// SESSION
	this->error = e;
	this->ostr = std::string("HTTP/1.1 ") + num_2_str(this->error) + std::string(" err description\r\n\r\nError Data\r\n");

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
		if (this->cgi.status(WNOHANG) > 0)
		{
			// ATTN : seems like we'd want to send an error here ...
			// assume it's been set (?)
			// keep-alive (?)
			// or .. let pollout take care of it 
			this->mod_evt(EPOLLOUT);
			// return (-1);
		}
		this->mod_evt(-EPOLLIN);
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	int req_state = sess.write(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);
		
// SESSION
// kd : integration
	// sess::rsrc = new ResourceCgi(conn!)
	if (this->cgi.pid == 0)
	{
		this->req_cnt++;
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec: cgi");
			this->set_err(503);
			// this->mod_evt(-EPOLLIN);
// or .. wait for cgi to send us data (?)
			// this->mod_evt(EPOLLOUT);
			return (0);
		}
		// chunked
		// std::string cont("HTTP/1.1 100 Continue\r\n\r\n");
		// this->send(cont);
	}
	// session::write .. 
		// should "pass" this to the resource
		// this code should be in there 
	// Each time we receive request data, let the cgi resource know
	if (this->cgi.ip)
	{
		// WsLog::_(LVL_ERR, TGT_BODY, "push: cgi");
		this->cgi.ip->mod_evt(EPOLLOUT);
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
		
// SESSION
// kd : integration
	//  How should we "switch" from ResourceCgi to ResourceError (send file ...)
	if (this->error)
	{
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->fd);
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  error ", this->error);
// SESSION
		err = this->send(this->ostr); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
		if (err < 0)
			return (-1);
		if (this->ostr.size())
			return (err);
		return (-1);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");


// ALL THIS : when requesting data from Session/Resource

// which will .. for cgi
// check cgi.hed
// 
	err = this->cgi_done(); // cgi_check();
	// (-1) : done : close / keep-alive
	// (0)  : in progress, no data
	// (1)  : have data to write
	if (err == 0)
		return (0);
	if (err < 0)
	{
// SESSION / REQUEST - move back to pollout
#if 0 // KEEP_ALIVE : WORKS, but requires CONTENT-LENGTH from (cgi)
// AND : we need to know that we have SENT everything to the client
		// resource::tlen
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  keep-alive ", this->req_cnt);
		this->sess.reset();
		this->cgi.reset();
		this->ostr.clear();
		this->mod_evt(-EPOLLOUT);
		this->mod_evt(EPOLLIN);
		return (0);
#else
		return (-1);		
#endif		
	}

// SESSION
// kd : integration
	//  the std::string ostr is flushed to the client,
	//  but may be filled by the CGI
#if 0
	if (ostr.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send:  ostr.size() == 0");

		err = this->cgi_done();
// KEEP_ALIVE
	// Session::reset
		return (err);
	}
#endif
// SESSION : fetch from resource .. if necessary
	// into fixed (ostr) here (?)
	// sess->fetch(str & ostr)
		// if (ostr.size())
			// return it 
		// else .. fill from .. stream
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
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent:  all");
		// this->mod_evt(-EPOLLOUT); 
	}
// SESSION
	// detect "done" from resource here (?)
	
	
	return (err); // (!) bytes written
}

// rdhup : may want to close (cgi.ip)
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
	if (this->cgi.hed == 0)
	{
		size_t	pos = ostr.find("\r\n\r\n");
		if (pos == std::string::npos)
			return (0);
			
		this->cgi.hed = 1;
		this->cgi.hlen = pos + 4;
// REQUEST
// kd : 

// WsLog::_(LVL_DBG, TGT_CGI_RECV, "head:");
// WsLog::_(LVL_DBG, TGT_CGI_RECV, "****\n", ostr);
		pos = ostr.find("Content-Length"); // case (!)
		if (pos != std::string::npos)
		{
			std::stringstream	line(ostr.substr(pos));
			std::string key;
			line >> key >> this->cgi.clen;
			WsLog::_(LVL_DBG, TGT_CGI_DATA, "clen: ", this->cgi.clen);
			this->cgi.tlen = this->cgi.hlen + this->cgi.clen;
			WsLog::_(LVL_DBG, TGT_CGI_DATA, "tlen: ", this->cgi.tlen);
			// plus HTTP STATUS LINE 
		}
		pos = ostr.find("Status"); // case (!)
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
		// we are able to "Keep-Alive"
		// if the CGI has provided "Content-Length"

// header.get(str, off)
	    std::stringstream	line(ostr.substr(pos));
		std::string key;
		std::string val;
		line >> key >> val;
		WsLog::_(LVL_DBG, TGT_CGI_DATA, "stat:  ", val);

		int http_stat = atoi(val.c_str());
		if (http_stat != 200)
			this->set_err(http_stat);
		else
			this->ostr.insert(0, std::string("HTTP/1.1 200 OK\r\n"));
	}
	return (0);
}

// called in ~CgiPipe()
// may trigger cgi.status(0)
// ResourceCgi (!)
void	Connection::cgi_rem(CgiPipe *epc)
{
	switch (this->cgi.rem(epc))
	{
	case 1: // (ip)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (ip) ", this->fd);
		// this->mod_evt(EPOLLOUT);
		break;
	case 2: // (op)
		WsLog::_(LVL_DBG, TGT_CONN, "rem : cgi (op) ", this->fd);
		this->mod_evt(EPOLLOUT);
		break;
	default:
		break;
	}
}

// ResourceCgi (!)
int		Connection::cgi_status(int opt)
{ 
	int	err;

	// may be sig .. 
	err = this->cgi.status(opt);
	if (err > 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "stat: ", this->cgi.xit);
		// OR : rsrc HOLDS ERROR
		// which conn chekcs at pollout
		if (this->cgi.xit == 2)
			this->set_err(404);
		else
			this->set_err(504);
	}
	return (err); 
}

// ResourceCgi (!)
int		Connection::cgi_done(void)
{
	if (!this->cgi.hed)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no cgi hed");
		this->mod_evt(-EPOLLOUT); 
		return (0);
	}
	
	// resource state
		// sent stat
		// sent head
		// sent body
		// unknown (no content-length : wait for cgi-close)
	if (ostr.size())
		return (1);
	// we just checked this in pollout
	if (this->cgi.status(WNOHANG) != -1)
	{
		 // CGI is DONE	
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
	if (this->cgi.op)
		this->cgi.op->mod_evt(EPOLLIN);
		
	this->mod_evt(EPOLLIN); // only if more body to send
	if (this->cgi.ip)
		this->cgi.ip->mod_evt(EPOLLOUT);
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
	

	// new ResourceCgi(pid, pipes, this)
	
	cgi.pid = pid;
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

	// (rsrc)
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
