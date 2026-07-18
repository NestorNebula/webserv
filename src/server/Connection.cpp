/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/18 17:48:29 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"


ResourceCgi::~ResourceCgi()
{
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "(~) ResourceCgi");
	if (this->ip || this->op) // pid, stat (?)
	{
		if (this->stat == -1)
		{
			kill(this->pid, SIGKILL);
			this->status(0);
		}
	}
	
	if (this->ip)
	{
		this->ip->conn_closed();
		this->ip->mod_evt(EPOLLIN);
	}
	if (this->op)
	{
		this->op->conn_closed();
		this->op->mod_evt(EPOLLOUT);
	}
}

// session::status()
// resource::status()
int	ResourceCgi::status(int opt)
{
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "pid : ", this->pid);
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "stat: ", this->stat);
	if (this->stat != -1)
		return (this->stat);
	if (this->pid == 0)
		return (this->stat);
	
	int err = waitpid(this->pid, &this->stat, opt);
	
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "wait: ", err);
	WsLog::_(LVL_DBG, TGT_CGI_RSRC, "stat: ", stat);

	// hm : (0) for (0)
	if (err == 0)
		return (this->stat); // WNOHANG : no change .. (-1)
	if (err < 0)
		WsLog::_errno(LVL_ERR, TGT_CGI_RSRC, "waitpid");
	if (WIFEXITED(stat))
	{
		WsLog::_(LVL_DBG, TGT_CGI_RSRC, "exit: ", WEXITSTATUS(stat));
		return (this->stat);
	}
	if (WIFSIGNALED(stat))
	{
		WsLog::_(LVL_DBG, TGT_CGI_RSRC, "sig : ", WTERMSIG(stat));
		return (this->stat);
	}
	return (this->stat);
}


void	ResourceCgi::rem(CgiPipe *epc)
{
	if (epc == this->ip)
	{
		this->ip = NULL;
	}
	else if (epc == this->op)
	{
		this->op = NULL;
		// this->mod_evt(EPOLLOUT); // conn
	}
	if (this->ip == NULL && this->op == NULL)
	{
		this->status(0);
		// may need to set conn->error
	}
}



Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	serv(_serv), 
	req_cnt(0),
	state(0)
{
};


Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
};

bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + EPC_TIMEOUT) < now)
	{
		this->set_err(408);
		return (true);
	}
	return (false);
}

void	Connection::set_err(int e)
{
	if (this->error)
	{
		WsLog::_(LVL_DBG, TGT_CONN, "err:  already set!");
	}
	this->error = e;
	this->resp = std::string("HTTP/1.1 ") + num_2_str(e) + std::string(" err description\r\n\r\nError Data\r\n");
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
	if (err == 0) // often with evt typ : in rdhup
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv:  ZERO");
		return (-1);
	}

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

// SESSION : write()
	int req_state = sess.push_data(this->ibuf, err);
	if (req_state < REQ_HAVE_HEAD)
		return (err);
		
	if (this->state < CONN_HAS_RSRC)
	{
		if (this->exec_cgi() < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			this->set_err(501);
			this->mod_evt(-EPOLLIN);
			this->mod_evt(EPOLLOUT);
			return (0);
		}
		this->state = CONN_HAS_RSRC;
	}
	
	if (this->cgi.ip)
		this->cgi.ip->mod_evt(EPOLLOUT);
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
	{
		if (this->cgi.status(WNOHANG) > 0)
			this->set_err(808);
	}
	if (this->error)
	{
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "err : ", err);
		if (err < 0)
			return (-1);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (-1);
	}

	// rsrc : not yet created 
	if (this->state < CONN_HAS_RSRC)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no rsrc");
		return (0);
	}

	// should cgi .. wait to find .. end of header .. before sending (?)

// Q: when should we create RESP .. for the cgi (?)
	if (this->state < RSRC_HAS_RESP) // or ERROR
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no resp");
		this->mod_evt(-EPOLLOUT);
		return (0);
	}
	
// UGH : php .. sends something .. even if it's going to quit with an error
	if (this->state < CONN_SENT_RESP) 
	{
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");


	// rsrc.complete
	std::string & odata = sess.read_data();
	
	if (odata.size() == 0) // rsrc/state seems like a better check
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: odata.size() == 0");

// UGLY
		if (this->cgi.status(WNOHANG) != -1)
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
	


	
	err = this->send(odata);
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
	if (odata.size())
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", odata.size());
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
	
// SESSION
int	Connection::req_body_status(void)
{
	if (this->sess.req.get_body().size())
		return (1);
	// if (body-fully-received) // ASSUMED
		return (-1);
	return (0);
}

// SESSION
int	Connection::push_resp_data(const char *buf, ssize_t siz)
{
	if (this->state < RSRC_HAS_RESP)
	{
		this->resp = std::string("HTTP/1.1 200 OK\r\n");
		this->state = RSRC_HAS_RESP;
	}
	std::string & odata = this->sess.read_data();
	odata.append(buf, siz);
	this->mod_evt(EPOLLOUT);
	
	WsLog::_(LVL_DBG, TGT_CGI_RECV, "odata: ", odata.size());
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "odata");
	WsLog::_(LVL_DBG, TGT_CGI_DATA, "****\n", odata);
	return (0);
}

int	Connection::exec_cgi(void)
{
	int			err;
	cgi_pipes	pipes;

	if (pipes.init() < 0)
	{
		this->set_err(707);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes.init");
	}

	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		this->set_err(707);
		return (-1);
	}
		
	cgi.pid = fork();
	if (cgi.pid < 0)
	{
		delete (cgienv);
		this->set_err(707);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}	
	if (cgi.pid == 0)
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
		pipes.dup_err();
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
	{
		this->set_err(707);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	}
	int cgifd_op = dup(pipes.p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		this->set_err(707);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	}	

	this->cgi.ip = new CgiPipe(this->ep, cgifd_ip, this);
	err = this->cgi.ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		this->set_err(707);
		return (err);
	}

	this->cgi.op = new CgiPipe(this->ep, cgifd_op, this);
	err = this->cgi.op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		this->set_err(707);
		return (err);
	}
	return (err);
}
