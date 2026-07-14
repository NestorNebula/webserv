/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/14 16:38:38 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	cgi_pid(0),
	cgi_ip(NULL),
	cgi_op(NULL),
	serv(_serv), 
	req_cnt(0),
	state(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);

	// Resource .. Session
	if (this->cgi_ip || this->cgi_op)
	{
		kill(cgi_pid, SIGKILL);
		int stat = 0;
		int err = waitpid(cgi_pid, &stat, 0);
		if (err < 0)
			WsLog::_errno(LVL_ERR, TGT_CONN, "waitpid");
		if (WIFEXITED(stat))
			WsLog::_(LVL_DBG, TGT_CONN, "exit: ", WEXITSTATUS(stat));
		if (WIFSIGNALED(stat))
			WsLog::_(LVL_DBG, TGT_CONN, "sig : ", WTERMSIG(stat));
	}
	
	if (this->cgi_ip)
	{
		this->cgi_ip->conn_closed();
		this->cgi_ip->mod_evt(EPOLLIN);
	}
	if (this->cgi_op)
	{
		this->cgi_op->conn_closed();
		this->cgi_op->mod_evt(EPOLLOUT);
	}
};

// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 

// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 

// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));		
}


std::string Connection::header(const char *key)
{
	std::string	kstr(key);
	std::string	val("");
	
	if (kstr == std::string("METH"))
	{
		std::string			meth;
		std::stringstream	line(head);
		line >> meth;
		WsLog::_(LVL_DBG, TGT_HEAD, "meth: ", meth);
		return (meth);
	}
	if (kstr == std::string("PATH"))
	{
		std::string			meth;
		std::string			path;
		std::stringstream	line(head);
		line >> meth >> path;
		WsLog::_(LVL_DBG, TGT_HEAD, "path: ", path);
		return (path);
	}
	if (kstr == std::string("QUERY"))
	{
		std::string			meth;
		std::string			path;
		std::stringstream	line(head);
		line >> meth >> path;
		size_t	qbeg = path.find('?');
		if (qbeg == std::string::npos)
			return (val);
		WsLog::_(LVL_DBG, TGT_HEAD, "getv: ", path.substr(qbeg + 1));
		return (path.substr(qbeg + 1));
	}
	
	std::string::const_iterator it = std::search(
		head.begin(), head.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == head.end())
	{
		// std::cerr << "header: not found\n";
		return (val);
	}
	it += strlen(key);
	if (*it != ':')
	{
		// std::cerr << "header: not colon\n";
		return (val);
	}
	it++;
	it++; // space (ugly) BUT IMPORTANT
	size_t off_beg = (it - head.begin());

	std::string hed_eol("\r\n");
	size_t	off_end = head.find(hed_eol, off_beg);

	if (off_end == std::string::npos)
	{
		return (val);
	}

	val = head.substr(off_beg, off_end - off_beg);

	std::string kv(key + std::string("=") + val);
	WsLog::_(LVL_DBG, TGT_HEAD, kv);
	return (val);
}



#define CONN_TIMEO 5


bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEO) < now)
		return (true);
	return (false);
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
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: zero");
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	
	sess.push_data(this->ibuf, err);
	
	// istr.append(this->ibuf, err);
	
	WsLog::_(LVL_DBG, TGT_CONN_RECV, "istr: ", istr.size());
	
	// Request
	if (this->state < CONN_HAS_HEAD)
	{
		std::string hed_end("\r\n\r\n");
		
		size_t	crlf = istr.find(hed_end);
		if (crlf == std::string::npos)
			return (err);
		
		head = istr.substr(0, crlf + 4);
		istr.erase(0, crlf + 4);
		
		WsLog::_(LVL_DBG, TGT_HEAD, "head");
		WsLog::_(LVL_DBG, TGT_HEAD, "****\n", head);
		// NULL HERE : sucks
		// WsLog::_(LVL_DBG, TGT_HEAD, "rest");
		// WsLog::_(LVL_DBG, TGT_HEAD, "****\n", istr);

		this->state = CONN_HAS_HEAD;
		this->req_cnt++;

		// GET : assume nothing more coming
		// this->mod_evt(-EPOLLIN);
	}

	// std::string cont = std::string("HTTP/1.1 100 Continue\r\n\r\n");
	// this->send(cont);	// ugly : have not checked (pollout)
	
#define KEEP_ALIVE 0

// Warning: Connection-specific header fields such as Connection and Keep-Alive are prohibited in HTTP/2 and HTTP/3. Chrome and Firefox ignore them in HTTP/2 responses, but Safari conforms to the HTTP/2 specification requirements and does not load any response that contains them.

	if (this->state < CONN_HAS_RSRC)
	{
		this->resp = std::string("HTTP/1.1 200 OK\r\n"); // UGLY
		switch(this->serv.get_port())
		{
		case 8080: // (php)
#if KEEP_ALIVE
			this->resp += std::string("Content-Length: 734\r\n");
			this->resp += std::string("Connection: Keep-Alive\r\n");
#else
			// this->resp = std::string("HTTP/1.1 200 OK\r\n");
#endif			
			break;
		default:
#if KEEP_ALIVE
			// this->resp += std::string("Content-Length: 594\r\n"); // one-shot
			this->resp += std::string("Content-Length: 698\r\n"); // concurrent
			this->resp += std::string("Connection: Keep-Alive\r\n");
#endif			
			this->resp += std::string("\r\n");
			break;
		}
		err = this->exec_cgi(); // (this->head) (sess)
		if (err < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			// different RESP here 
			return (err);
		}
		this->state = CONN_HAS_RSRC;
	}
	// sess.got data -- more body
	if (this->cgi_ip)
	{
		// if (err < EPC_BUF_SIZ) // all data received (?)
		// {
			// tell (cgi_ip) ?
			// close (rd) on this socket (?)
		// }
		// state : conn_have_body
		// check post-epoll state -- set in (cur_evt)
		// and .. semd immediately (?)
		this->cgi_ip->mod_evt(EPOLLOUT);
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

	if (this->state < CONN_HAS_RSRC)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no rsrc");
		return (0); // (-1)
	}

	if (this->state == RSRC_BODY_DONE) //  || this->cgi_op == NULL) // complete
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: cgi DONE");
		int stat = 0;
		err = waitpid(cgi_pid, &stat, 0);
		if (WIFEXITED(stat))
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "EXIT: ", WEXITSTATUS(stat));
		if (WIFSIGNALED(stat))
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "SIG : ", WTERMSIG(stat));
		// what .. we were sending .. but then there was an error (?)
		if (!WIFEXITED(stat) || WEXITSTATUS(stat))
		{
			this->state = RSRC_ERROR;
			// something to send 
			this->resp = std::string("HTTP/1.1 501 Not Implemented\r\n\r\nError");
			this->state = RSRC_HAS_RESP;
		}
	}
	
	if (this->state < RSRC_HAS_RESP)
	{
		// get this more than I like ... 
		//back to mod_evt(-)
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no resp");
		this->mod_evt(-EPOLLOUT); // why not (?)
		// should work with (-EPOLLOUT) .. right (?)
		// BUT : we want (0) to mean (0) ... 
		return (0); // (-1)
	}

	// don't send resp .. until we have some body
	// cgi may have FAILED
	
	if (this->state < CONN_SENT_RESP || this->state == RSRC_BODY_DONE)
	{
		// BODY_DONE : could be .. complete with error 
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");

	// rsrc.data
	
	// FUCK : cgi may have hung up and deleted itself

	// AND : cgi .. has been sending data (?)
	if (ostr.size() == 0) // rsrc/state seems like a better check
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");

		// rsrc.inProgress() .. wait for it
		// rsrc.failure() .. send error
		// rsrc.done()
		// not always getting RSRC_BODY_DONE 
		// should even check .. before SENDING (ostr)
		if (this->state == RSRC_BODY_DONE || this->cgi_op == NULL) // complete
		{
			// int stat = 0;
			// err = waitpid(cgi_pid, &stat, 0);
			// if (WIFEXITED(stat))
			// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "exit: ", WEXITSTATUS(stat));
			// if (WIFSIGNALED(stat))
			// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sig : ", WTERMSIG(stat));

// Keepalive : with chunked transfer encoding

// Keepalive makes it difficult for the client to determine where one response ends and the next response begins, particularly during pipelined HTTP operation.[11] This is a serious problem when Content-Length cannot be used due to streaming.[12] To solve this problem, HTTP 1.1 introduced a chunked transfer coding that defines a last-chunk bit.[13] The last-chunk bit is set at the end of each response so that the client knows where the next response begins.
#if KEEP_ALIVE // -- would REQUIRE CONTENT-LENGTH from (cgi)

			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: keep-alive");
			// hm : did this only work when we sent back Content-Length
			// see more HUP => read [0] with THIS .. AND NOT siege.conf
			this->istr.clear();
			this->state = 0;
			this->mod_evt(-EPOLLOUT); // no rsrc
			this->mod_evt(EPOLLIN);
			return (0);
#else
			// TEST : content-length header .. longer than what we send
			return (-1);		
#endif			
		}
		// lucky .. send has probably already happened 
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (0);
	}
	
	// rsrc.data_op
	err = this->send(ostr);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: zero");
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	if (ostr.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
	else
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: all");

	return (err); // (!) bytes written
}







// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read.
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
	

// Resource .. built from Request
// CgiEnv   .. built from Request .. add to current ENV (?)

// sess.rsrc_cgi
void	Connection::rem_cgi(CgiPipe *epc)
{
	// Q: error message (?)
	if (epc == this->cgi_ip) // expect this to close before (op)
		this->cgi_ip = NULL;
	else if (epc == this->cgi_op)
		this->cgi_op = NULL;
	// both deleted .. check (exit) status (?)
}

// new rsrc (cgi)
int	Connection::exec_cgi(void)
{
	int			err;
	cgi_pipes	pipes;

	if (pipes.init() < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes");

	this->cgi_pid = fork();
	if (cgi_pid < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
		
	if (cgi_pid == 0)
	{
		err = pipes.dup_io();
		if (err < 0)
		{
			pipes.shutdown();
			delete (this->ep);
			exit(1);
		}

// FROM REQUEST
		std::string	path;
		std::string	file;
		
		switch(this->serv.get_port())
		{
		case 8081:
			path = std::string("/usr/bin/python"); // exec
			file = std::string("test.py");
			break;
		case 8082:
			path = std::string("/usr/bin/perl");
			file = std::string("test.pl");
			break;
		default:
			path = std::string("/usr/bin/php-cgi"); // HOME : fail better
			file = std::string("test.php");
			break;
		}
		
		char const *args[3];
		args[0] = path.c_str();
		args[1] = file.c_str();
		args[2] = NULL;

// CHALLENGE
// terminate long-running script if client closes connection
		CgiEnv *cgienv = new CgiEnv;
		// from_req : exec
		cgienv->from_conn(*this, file);

		const char **envp = cgienv->gen();

		signal(SIGINT, SIG_DFL);
		err = execve(args[0], (char* const*) args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	

	
	WsLog::_(LVL_DBG, TGT_CONN, "exec cgi");

	// cgi resource : needs conn .. 
	// unless it just uses its "parent" resource 
	int			cgifd_ip;
	int			cgifd_op;
	
	cgifd_ip = dup(pipes.p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	cgifd_op = dup(pipes.p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	}	

	this->cgi_ip = new CgiPipe(this->ep, cgifd_ip, this);
	err = this->cgi_ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->cgi_op = new CgiPipe(this->ep, cgifd_op, this);
	err = this->cgi_op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	return (err);
}
