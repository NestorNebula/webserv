/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/16 23:45:02 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "Session.hpp"

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	sess(_serv.get_conf()),
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
	// sess->close()
	// rsrc.active()

	// delete (ResourceCgi) ? 
	if (this->cgi_ip || this->cgi_op)
	{
		kill(cgi_pid, SIGKILL);
		
		int stat = -1;
		int err = waitpid(cgi_pid, &stat, 0);
		WsLog::_(LVL_DBG, TGT_CONN, "wait: ", err);
		WsLog::_(LVL_DBG, TGT_CONN, "stat: ", stat);
		if (err < 0)
			WsLog::_errno(LVL_ERR, TGT_CONN, "waitpid");
		if (WIFEXITED(stat))
			WsLog::_(LVL_DBG, TGT_CONN, "exit: ", WEXITSTATUS(stat));
		if (WIFSIGNALED(stat))
			WsLog::_(LVL_DBG, TGT_CONN, "sig : ", WTERMSIG(stat));
		// what is the point of capturing exit status here ..
		// connection is dying
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


// session::status()
// resource::status()
int	Connection::cgi_status(void)
{
	// if (cgi_stat >= 0)
		// DONE
	if (cgi_pid == 0)
		return (0);
	if (cgi_ip == NULL && cgi_op == NULL)
		return (0); // hm : assume done (?) don't think so
	int stat = -1;
	int err = waitpid(cgi_pid, &stat, WNOHANG);
	WsLog::_(LVL_DBG, TGT_CONN, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_CONN, "stat: ", stat);
	if (err == 0)
		return (0);
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_CONN, "waitpid");
	if (WIFEXITED(stat))
	{
		WsLog::_(LVL_DBG, TGT_CONN, "exit: ", WEXITSTATUS(stat));
		return (1);
	}
	if (WIFSIGNALED(stat))
	{
		WsLog::_(LVL_DBG, TGT_CONN, "sig : ", WTERMSIG(stat));
		return (1);
	}
	return (0);
}

#define CONN_TIMEO 5


bool	Connection::timeo(time_t now) const
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEO) < now)
	{
		// 408 Request Timeout -- set error code 
		return (true);
	}
	return (false);
}

ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	// sess.status()

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
		return (err);
	}
	if (err == 0) // often with evt typ : in rdhup
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ZERO");
		// return (-1);
		this->mod_evt(-EPOLLIN);
		return 0;
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

	// sess.write_data()
	// set some 
	if (sess.nextAction() == Session::RDSOCK)
		sess.write(this->ibuf, err);
	switch (sess.nextAction()) {
		case Session::DOCGI:
			if (this->exec_cgi() < 0)
			{
				WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
				this->error = 501; // session (?)
				this->mod_evt(-EPOLLIN);
				this->mod_evt(EPOLLOUT);
				return (0);
			}
			if (this->cgi_ip)
				this->cgi_ip->mod_evt(EPOLLOUT);
			break;
		case Session::WRSOCK:
			this->mod_evt(EPOLLOUT);
			break;
		case Session::RDSOCK:
		case Session::KPALIVE:
		case Session::CLOSE:
			break;
	}
	// rsrc.input_data_available
	// in sess.push_data()
	// sess .. knows about rsrcCgi .. 
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
	// sess.get_state()
	// rsrc.get_state()
	/*
	if (this->error)
	{
		// set (resp)
		// or .. rsrc .. prepares this (?)
		this->resp = std::string("HTTP/1.1 501 Not Implemented\r\n\r\nError");

		// which may be an error .. page 
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (-1);
		
		// and SEND NOW 
	}
	*/

	// rsrc died by errror .. and we need to send it 
	/*
	if (this->state < CONN_HAS_RSRC)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no rsrc");
		return (0);
	}
	*/

	// rsrc.state / error 
	// error
	// osiz
	// osiz == 0 .. but inProgress
	// rsrc.done -- not BODY done 
	
	// which we could have gotten .. 
	// when the second got (rem_cgi)
	// or .. EPOLLERR

	// rsrc.state()
	
	// sess.check_state() -- could have tested (cgi) for errors
	if (sess.nextAction() != Session::WRSOCK)
		return 0;
    if (!this->cgi_ip && !this->cgi_op)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: cgi DONE");

// BUT : we may have some data to send
// AND : we may not yet have sent RESP
// track cgi_status .. so we only do this once 
		int stat = 0;
		err = waitpid(cgi_pid, &stat, 0);
		

		WsLog::_(LVL_DBG, TGT_CONN, "wait: ", err);
		WsLog::_(LVL_DBG, TGT_CONN, "stat: ", stat);
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
	
	/*
	if (this->state < RSRC_HAS_RESP) // or ERROR
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no resp");
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	*/
	
	/*
	if (this->state < CONN_SENT_RESP) 
	{
		// which may be an error .. page 
		// resp : is built -- and sent bit-by-bit
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (0);
	}
	*/
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");


	// rsrc.complete
	char buf[4096];
	Stream::streamsize r = sess.read(buf, 4096);
	if (r > 0)
		err = this->send(buf, r);
	if (sess.nextAction() != Session::WRSOCK) // rsrc/state seems like a better check
	{
		//WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: odata.size() == 0");

// UGLY
		// if (this->cgi_op == NULL) // complete
		{

// Keepalive : with chunked transfer encoding
// Keepalive makes it difficult for the client to determine where one response ends and the next response begins, particularly during pipelined HTTP operation.[11] This is a serious problem when Content-Length cannot be used due to streaming.[12] To solve this problem, HTTP 1.1 introduced a chunked transfer coding that defines a last-chunk bit.[13] The last-chunk bit is set at the end of each response so that the client knows where the next response begins.
// #if KEEP_ALIVE // -- would REQUIRE CONTENT-LENGTH from (cgi)
			if (sess.nextAction() == Session::KPALIVE) {

			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: keep-alive");
			// hm : did this only work when we sent back Content-Length
			// see more HUP => read [0] with THIS .. AND NOT siege.conf
			// this->istr.clear();
			// this->state = 0;
			this->sess.reset();
			this->mod_evt(-EPOLLOUT); // no rsrc
			this->mod_evt(EPOLLIN);
			return (0);
			}
			// TEST : content-length header .. longer than what we send
			// return (-1);		
		}
		// LUCKY .. send has probably already happened 
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (-1);
	}
	


	
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
	if (r)
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", r);
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

// sess.has_input
int	Connection::cgi_inp(void)
{
	// cgi_valid (?)
	if (this->sess.nextAction() == Session::DOCGI && this->sess.getRequest().hasBody())
		return (1);
	// if (req) has NOT received full body
	this->mod_evt(EPOLLOUT);
	return (0);
	// else
	// body.size() == 0 AND there is nothing more to receive
	// return (-1);
}

// rsrc.odata()
// sess
// resource .. fills some output_buffer in session
int	Connection::cgi_out(const char *buf, ssize_t siz)
{
	/*
	 Can't compile with that code.
	if (this->state < RSRC_HAS_RESP)
	{
		// assume success if we get here (?)
		// Python and Perl need to add "\r\n" manually
		// PHP - may have added it's own error header, though
		this->resp = std::string("HTTP/1.1 200 OK\r\n");
		this->state = RSRC_HAS_RESP;
	}
	std::string & odata = this->sess.read_data();
	odata.append(buf, siz);
	this->mod_evt(EPOLLOUT);
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "odata: ", odata.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "odata");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", odata);
	*/

	(void) buf;
	(void) siz;
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
	// if both null -- check status
}


// this->sess.rsrsc = new ResourceCgi(this);
int	Connection::exec_cgi(void)
{
	int			err;
	cgi_pipes	pipes;

	if (pipes.init() < 0)
		// this->set_err()
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes.init");

	// or : post-fork .. and .. error is detected .. elsewhere (?)
	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI, "cgienv: FAIL");
		// pipes.shutdown();
		delete (cgienv);
		return (-1);
	}
		
	this->cgi_pid = fork();
	if (cgi_pid < 0)
	{
		// pipes.shutdown();
		delete (cgienv);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}	
	if (cgi_pid == 0)
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
		// pipes.dup_err();
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
	
		exit (err);
	}		
	delete (cgienv);
	
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
