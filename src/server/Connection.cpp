/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/14 20:59:16 by kdonlon          ###   ########.fr       */
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




#define CONN_TIMEO 5


bool	Connection::timeo(time_t now) const
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEO) < now)
		return (true);
	// 408 Request Timeout -- set error code 
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
		// often with evt typ : in rdhup
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ZERO");
		return (-1);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	int req_state = sess.push_data(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);
		
	if (this->state < CONN_HAS_RSRC)
	{
		err = this->exec_cgi();
		if (err < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			// SEND RESP (!)
			this->error = 501;
			this->mod_evt(-EPOLLIN);
			return (0);
		}
		this->state = CONN_HAS_RSRC; // but body might not be done 
	}
	
	if (this->cgi_ip)
		this->cgi_ip->mod_evt(EPOLLOUT);
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

	// check_state

	// rsrc.get_state()
	if (this->error)
	{
		// set (resp)
		this->resp = std::string("HTTP/1.1 501 Not Implemented\r\n\r\nError");
	}

	if (this->state < CONN_HAS_RSRC)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no rsrc");
		return (0);
	}

	// rsrc.state / error 
	// error
	// osiz
	// osiz == 0 .. but inProgress
	// rsrc.done -- not BODY done 
	
	// which we could have gotten .. 
	// when the second got (rem_cgi)
	// or .. EPOLLERR
	if (!this->cgi_ip && !this->cgi_op)
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
			this->error = 501;
			// ugh -- we'll check this .. AGAIN
			return (0);
			// something to send 
			this->resp = std::string("HTTP/1.1 501 Not Implemented\r\n\r\nError");
			this->state = RSRC_HAS_RESP; // UGLY
		}
	}
	
	if (this->state < RSRC_HAS_RESP) // or ERROR
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no resp");
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	
	if (this->state < CONN_SENT_RESP) 
	{
		// which may be an error .. page 
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");

	if (ostr.size() == 0) // rsrc/state seems like a better check
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");

		if (this->cgi_op == NULL) // complete
		{

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
		// LUCKY .. send has probably already happened 
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (0);
	}
	


	
	err = this->send(ostr);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ZERO");
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

int	Connection::cgi_inp(void)
{
	if (this->sess.req.get_body().size())
		return (1);
	// if (req) has NOT received full body
	this->mod_evt(EPOLLOUT);
	return (0);
	// else
	// body.size() == 0 AND there is nothing more to receive
	// return (-1);
}

int	Connection::cgi_out(const char *buf, ssize_t siz)
{
	if (this->state < RSRC_HAS_RESP)
	{
		// assume success if we get here (?)
		// Python and Perl need to add "\r\n" manually
		// PHP - may have added it's own error header, though
		this->resp = std::string("HTTP/1.1 200 OK\r\n");
		this->state = RSRC_HAS_RESP;
	}
	this->ostr.append(buf, siz);
	this->mod_evt(EPOLLOUT);
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "ostr: ", this->ostr.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "ostr");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", this->ostr);
	return (0);
}

// check epc->error (?)
void	Connection::rem_cgi(CgiPipe *epc)
{
	if (epc == this->cgi_ip)
	{
		this->cgi_ip = NULL;
	}
	else if (epc == this->cgi_op)
	{
		this->cgi_op = NULL;
		this->mod_evt(EPOLLOUT);
	}
	// both deleted .. check (exit) status (?) for error (?)
}


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
		CgiEnv *cgienv = new CgiEnv;
		err = cgienv->from_conn(*this);
		if (err < 0)
		{
			pipes.shutdown();
			delete (cgienv);
			delete (this->ep); 
			exit(1);
		}

		const char **envp = cgienv->gen();

		signal(SIGINT, SIG_DFL);
		// pipes.dup_err();
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	
	// this->rsrc = new ResourceCgi(this, &pipes);
	
	WsLog::_(LVL_DBG, TGT_CONN, "exec cgi");

	int cgifd_ip = dup(pipes.p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	int cgifd_op = dup(pipes.p2[0]);
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
