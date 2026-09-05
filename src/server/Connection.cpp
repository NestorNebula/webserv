/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/05 20:17:13 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"
#include "ResourceFcgi.hpp"
#include "ResourcePiped.hpp"

Connection::Connection	(const Connection & that) :
	EpollClient(that),
	sess(that.serv.get_conf()),
	serv(that.serv),
	req_cnt(0)
{

}

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) :
	EpollClient(_ep, EPC_CONN, _fd),
	sess(_serv.get_conf()),
	serv(_serv),
	retry_cgi(0),
	res_cgi(NULL),
	req_cnt(0)
{
};

Connection::~Connection()
{
	// WSLOG(LVL_DBG, TGT_CONN, " (~) Connection ", this->fd);
	// WSLOG(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
// KEEP_ALIVE : check req_cnt
	WSLOG(LVL_TMP, TGT_CONN, " (~) Connection ", this->fd);
	WSLOG(LVL_TMP, TGT_CONN, "req cnt: ", this->req_cnt);
	try
	{
		this->serv.conn_closed();
		if (this->res_cgi)
		{
			this->res_cgi->conn_closed();
			delete (this->res_cgi);
		}
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, " (~) Connection\n", e.what());
	}
}

bool	Connection::timeo(WsTime & now)
{
	if (this->lact.not_set())
		return (false);
	if (this->lact.after(now))
		return (false);
// sess  : Request Resource resolved: ./html/contact.html
// sess  : Checking operation is possible on Session Resource
// sess  : Operation possible on Session Resource
// sess  : Preparing Session Resource generation
// sess  : Processing upload Request
// res   : BuiltinResource constructor for code  [403]
// sess  : Generating Session Resource
// res   : Generating BuiltinResource for code  [403]
// res   : BuiltinResource called with wrong code, teapot found
// res   : BuiltinResource error for code  [418]
// res   : BuiltinResource destructor for code  [418]
	if ((sess.nextAction() == Session::RETRY) && ((this->lact + CGI_RETRY_INTERVAL).before(now)))
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_TMP, TGT_CONN | TGT_TIMEO | TGT_RETRY, "sess: retry");
		sess.manageSession();
		switch (sess.nextAction())
		{
		case Session::WRSOCK:
			this->req_cnt++;
			this->mod_evt(EPOLLOUT);
			break;
		default:
			break;
		}
		return (false);
	}
	if (retry_cgi && ((this->lact + CGI_RETRY_INTERVAL).before(now)))
	{
		this->lact = now;
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_TMP, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		if (this->exec_cgi() < 0)
		{
			if (retry_cgi >= MAX_RETRIES)
			{
				WSCOL(WSL_RED);
				WSLOG(LVL_TMP, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
// if (Connection) blocks all available (fd) ..
// we'd rather sacrifice JUST ONE ...
				this->set_err(504); // #kd (610)
			}
			retry_cgi++;
			return (0);
		}
		// success
		WSCOL(WSL_GREEN);
		WSLOG(LVL_TMP, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		this->retry_cgi = 0;
		this->mod_evt(EPOLLIN);
		return (0);
	}

	if ((this->lact + CONN_TIMEOUT).after(now))
		return (false);

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_TMP, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : conn ",
		// this->get_fd());
		evt_type(this->evt.events));
	// Request Timeout -- not necessarily
	// perhaps .. only if "pollin"
	// timeout .. on .. keep-alive ..
	// which is .. waiting for more mp3 data ...
	// Brave
	// php -- still re-setting EPOLLIN
	// keep-alive (file!) is strange here
// conn  : TIMEO : conn in rdhup
// conn  : TIMEO : conn in rdhup
// conn  : TIMEO : conn out rdhup

// keep-alive (?)
// which has not sent all data ..
// sess._next ..
		// EPOLLIN is the one to test
	// or .. if request is not complete ..
// sess  : Session::getRequest called while Session is in WRSOCK
// Session::getRequest should only be called in DOCGI mode. Returning raw request

// keep-alive : should just shut-down
	// if (!this->sess.getRequest().isComplete())
	if (this->sess.nextAction() == Session::RDSOCK)
	{
		this->set_err(408);
		return (true);
	}
	if (this->evt.events & EPOLLOUT)
	{
		this->lact = now;
		return (false);
	}
	// this->set_err(408);
	return (false);
}

int	Connection::set_err(int e)
{
	if (e == 0)
		return (-1);
	if (this->error)
	{
		WSLOG(LVL_ERR, TGT_CONN, "err:  already set!");
		WSLOG(LVL_ERR, TGT_CONN, "cur:  ", this->error);
		WSLOG(LVL_ERR, TGT_CONN, "new:  ", e);
		this->mod_evt(EPOLLOUT);
		return (-1);
	}
	WSLOG(LVL_DBG, TGT_CONN, "err:  ", e);
	try
	{
		this->error = e; // why not (?)
		this->sess.setError(e);
		this->mod_evt(EPOLLOUT);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (-1);
}

#if 0
static void sess_log_next(Session &sess)
{
    WSCOL(WSL_YELLOW);
    switch(sess.nextAction())
    {
    case Session::RDSOCK:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  RDSOCK");
      break;
    case Session::DOCGI:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  DOCGI");
      break;
    case Session::WRSOCK:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  WRSOCK");
      break;
    case Session::CLOSE:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  CLOSE");
      break;
    case Session::KPALIVE:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  KPALIVE");
      break;
    }
}
#endif

ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	try
	{
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  POLLIN");
		// sess_log_next(sess);
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv");
		err = this->recv();
		if (err < 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_RECV, "recv", err);
			return (err);
		}
		if (err == 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  ZERO");
			this->mod_evt(EPOLLOUT);
			return (0);
		}
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv: ", err);

		// sess_log_next(sess);
		switch(sess.nextAction())
		{
		case Session::RDSOCK:
		case Session::DOCGI:
			sess.write(this->ibuf, err);
			break;
		case Session::CLOSE:
			return (-1);
		default:
			break;
		}

		// may have got rdhup .. but still need to retry ..
		switch (sess.nextAction())
		{
		case Session::RETRY:
			WSCOL(WSL_CYAN);
			WSLOG(LVL_TMP, TGT_CONN | TGT_RETRY, "sess: RETRY");
			this->mod_evt(0);
			break;
		case Session::DOCGI:
			err = this->exec_cgi();
			if (err < 0)
			{
				if (err == SYSCALL_ERR)
				{
					WSCOL(WSL_CYAN);
					WSLOG(LVL_DBG, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "exec failed", retry_cgi);
					this->serv.set_paused(); // failed : CGI
					retry_cgi++;
					// ah -- but not retriggered in time
					// must retrigger .. soon (!)
					// only makese sense to re-try ..
					// once another has FLUSHED
					// PROBLEM : all open (fd) filled with Conn
					// ANSWER : one must give way
					// that is the key
					// cgi -- must try again
					// BEFORE Server ...
					this->mod_evt(0);
				}
				return (0); // send error
			}
			this->res_cgi->push_body();
			break;
		case Session::WRSOCK:
			this->req_cnt++;
			this->mod_evt(-EPOLLIN);
			this->mod_evt(EPOLLOUT);
			break;
		case Session::RDSOCK:
			break;
		case Session::KPALIVE:
			WSCOL(WSL_PURPLE);
			WSLOG(LVL_DBG, TGT_CONN, "keep-alive (ip)");
			// no reset here (?)
			return (0);
		case Session::CLOSE:
			return (-1);
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, "ex: pollin\n", e.what());
		this->set_err(404); // File Not Found
	}
	return (0);
}

ssize_t	Connection::pollout(void)
{
	ssize_t	err = 0;
	try
	{
		WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  POLLOUT");
		WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
		// sess_log_next(sess);
		if (sess.nextAction() == Session::DOCGI)
		{
			ResourceCgi *res = this->res_cgi;
			if (res == NULL)
			{
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (NULL)");
				return (-1);
			}
			switch (res->status())
			{
			case RSP_COMPLETE:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (< 0)");
				return (-1);
// KEEP_ALIVE
			case RSP_KPALIVE:
				WSCOL(WSL_PURPLE);
				WSLOG(LVL_DBG, TGT_CONN, "keep-alive (rsp) ", this->req_cnt);
				this->reset();
				return (0);
			case RSP_WAIT_HEAD:
			case RSP_WAIT_BODY:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  no data");
				this->mod_evt(-EPOLLOUT);
				return (0);
			case RSP_ERROR:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "res : (error)");
				return (0);
			default:
				break;
			}

			std::string & RESP = res->get_resp();

			WSLOG(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "resp");
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);
			err = this->send(RESP);
		}
		else
		{
			switch (sess.nextAction())
			{
			case Session::CLOSE:
				return (-1);
			case Session::WRSOCK:
			default:
				std::string & RESP = sess.getResponse();
				if (RESP.size())
				{
					WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
					WSLOG(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
					// WSLOG(LVL_DBG, TGT_CONN_SEND, "resp");
					// WSLOG(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);
					err = this->send(RESP);
				}
			}
		}

		if (err < 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
			return (err);
		}
		if (err == 0)
		{
			WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  ZERO");
			return (0);
		}
		WSLOG(LVL_DBG, TGT_CONN_SEND, "sent: ", err);

		// sess_log_next(sess);
		switch (sess.nextAction())
		{
		case Session::KPALIVE:
			WSCOL(WSL_PURPLE);
			WSLOG(LVL_DBG, TGT_CONN, "keep-alive (op) ", this->req_cnt);
			this->reset();
			return (0);
		case Session::CLOSE:
			return (-1);
		default:
			break;
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSLOG(LVL_DBG, TGT_CONN, "ex: pollout\n", e.what());
		this->set_err(404); // File Not Found
	}
	return (0);
}

int	Connection::rdhup(void)
{
	WSLOG(LVL_DBG, TGT_CONN, "RDHUP");
	this->mod_evt(EPOLLOUT);
	return (-1);
}

int	Connection::hup(void)
{
	WSLOG(LVL_DBG, TGT_CONN, "hup!");
	if (this->res_cgi)
		this->res_cgi->conn_closed();
	return (-1);
}

void	Connection::reset(void)
{
	if (this->res_cgi)
	{
		this->res_cgi->conn_closed();
		delete (this->res_cgi);
		this->res_cgi = NULL;
	}

	this->sess.reset();
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

void	Connection::cgi_rem(EpollClient *epc)
{
	switch (this->res_cgi->rem(epc))
	{
	case RSRC_DONE_IP:
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (ip)   ", this->fd);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case RSRC_DONE_OP:
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (op)   ", this->fd);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (op)   ", this->res_cgi->error);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	case RSRC_DONE_IO:
		WSLOG(LVL_DBG, TGT_CONN, "rem cgi  : (DONE) ", this->fd);
		WSLOG(LVL_DBG, TGT_CONN, "rem err  : (conn) ", this->error);
		this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		break;
	default:
		break;
	}
}

int	Connection::exec_cgi(void)
{
	if (this->res_cgi)
		return (0);

	this->req_cnt++;

	int			err;

	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WSLOG(LVL_DBG, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (this->set_err(500)); // #kd (601)
	}

	std::string &fcgi_sock = this->serv.get_conf().fcgi_sock;
	if (
		(cgienv->lang == CGI_PHP) &&
		!fcgi_sock.empty() &&
		!access(fcgi_sock.c_str(), R_OK | W_OK)
	)
	{
		ResourceFcgi * fcgi = new ResourceFcgi;
		err = fcgi->init(this->ep, cgienv, this, fcgi_sock);
		if (err == 0)
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_CONN, "init:  FCGI");
			delete (cgienv);
			this->res_cgi = fcgi;
// KEEP_ALIVE : set from Request (fcgi)
			this->res_cgi->ka = this->sess.getRequest().keepalive();
			return (err);
		}
		delete (cgienv);
		delete (fcgi);
		// ASSUMES : fail = "Too many open files"
		WSCOL(WSL_CYAN);
		WSLOG(LVL_TMP, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		return(SYSCALL_ERR);
	}

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_CONN, "php :  pipe");

	cgi_pipes	pipes;

	if (pipes.init() < 0)
	{
		delete (cgienv);
		WSCOL(WSL_CYAN);
		WSLOG(LVL_TMP, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		return (SYSCALL_ERR);
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		pipes.shutdown();
		delete (cgienv);
		WsLog::_errno(LVL_SYSERR, TGT_CONN, "fork");
		return (SYSCALL_ERR);
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
		if (err < 0)
		{
			pipes.shutdown();
			delete (cgienv);
			delete (this->ep);
			exit(1);
		}

		const char **envp = cgienv->gen();
		std::string & cwd = cgienv->get("CWD");
		if (cwd.size())
		{
			// WSCOL(WSL_GREEN);
			// WSLOG(LVL_DBG, TGT_CGI, "cwd : ", cwd);
			err = chdir(cwd.c_str());
			if (err < 0)
			{
				WsLog::_errno(LVL_SYSERR, TGT_CGI_ENV, "chdir");
				pipes.shutdown();
				delete (cgienv);
				delete (this->ep);
				exit(1);
			}
		}
		err = execve(cgienv->args[0], (char* const*) cgienv->args, (char* const*) envp);

		pipes.shutdown();
		delete (cgienv);
		delete (this->ep);

		exit (err);
	}
	delete (cgienv);

	ResourcePiped * pcgi = new ResourcePiped;
	err = pcgi->init(this->ep, pid, &pipes, this);
	if (err < 0)
	{
		pipes.shutdown();
		delete (pcgi);
		WSCOL(WSL_CYAN);
		WSLOG(LVL_TMP, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		return (SYSCALL_ERR);
	}
	this->res_cgi = pcgi;
// KEEP_ALIVE : set from Request (cgi)
	this->res_cgi->ka = this->sess.getRequest().keepalive();
	return (err);
}
