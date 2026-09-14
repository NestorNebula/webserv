/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/14 17:38:13 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"
#include "ResourceCgi.hpp"
#include "ResourceFcgi.hpp"
#include "ResourcePiped.hpp"
#include "Socket.hpp"

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
	WSLOG(LVL_DBG, TGT_CONN | TGT_KEEPA, " (~) Connection ", this->fd);
	WSLOG(LVL_DBG, TGT_CONN | TGT_KEEPA, "req cnt: ", this->req_cnt);
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

#define DBG_SESS_NEXT 0

#if DBG_SESS_NEXT
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
#if WITH_RETRY
    case Session::RETRY:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  RETRY");
      break;
#endif
    }
}
#endif

bool	Connection::timeo(WsTime & now)
{
	if (this->lact.not_set())
		return (false);
	if (this->lact.after(now))
		return (false);
#if WITH_RETRY
// RETRY : static resource
	if ((sess.nextAction() == Session::RETRY) && ((this->lact + CGI_RETRY_INTERVAL).before(now)))
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "sess: retry");
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
// RETRY : CGI resource
	if (retry_cgi && ((this->lact + CGI_RETRY_INTERVAL).before(now)))
	{
		this->lact = now;
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		if (this->exec_cgi() < 0)
		{
			if (retry_cgi >= MAX_RETRIES)
			{
				WSCOL(WSL_RED);
				WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
				this->set_err(504);
			}
			retry_cgi++;
			return (0);
		}
		WSCOL(WSL_GREEN);
		WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
		this->retry_cgi = 0;
		this->mod_evt(EPOLLIN);
		return (0);
	}
#endif

	if ((this->lact + CONN_TIMEOUT).after(now))
		return (false);

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : conn ", this->req_cnt);
	// WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : conn ", evt_type(this->evt.events));

	if (this->evt.events & EPOLLOUT)
	{
		WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : conn writing");
		this->lact = now;
		return (false);
	}

#if DBG_SESS_NEXT
	sess_log_next(sess);
#endif
	if (this->sess.nextAction() == Session::CLOSE)
	{
		this->lact = now;
		this->mod_evt(EPOLLOUT); // trigger fail (?)
		return (true);
	}
	if (this->sess.nextAction() == Session::RDSOCK)
	{
		WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : rdsock");
		if (this->req_cnt) // keep-alive timeout
		{
			WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : keep-alive");
			return (true);
		}
		else
		{
			// WSLOG(LVL_WARN, TGT_CONN | TGT_TIMEO | TGT_RETRY, "TIMEO : error");
			// It's like .. firefox opens sockets ..
			// that it does not use right away ...
			// so .. I send on this ..
			// AND : do not CLOSE IMMEDIATELY (fucking upload)
			// so .. strange
			this->set_err(408);
			this->mod_evt(-EPOLLIN);
			this->mod_evt(EPOLLOUT); // set_err should have done this
			return (false);
		}
	}
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
		// this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
		return (-1);
	}
	WSLOG(LVL_DBG, TGT_CONN, "err:  ", e);
	try
	{
		this->error = e; // why not (?)
		this->sess.setError(e);
		// this->mod_evt(-EPOLLIN);
		this->mod_evt(EPOLLOUT);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (-1);
}


#define CLOSE_ON_CLOSE 1

ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	try
	{
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  POLLIN");
		WSLOG(LVL_DBG, TGT_CONN_RECV, "recv");
		if (sess.nextAction() == Session::CLOSE)
		{
			WSLOG(LVL_DBG, TGT_CONN_RECV, "recv:  CLOSE");
#if CLOSE_ON_CLOSE
			return (-1);
#endif
		}
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

#if DBG_SESS_NEXT
		sess_log_next(sess);
#endif
		switch(sess.nextAction())
		{
		case Session::RDSOCK:
		case Session::DOCGI:
			sess.write(this->ibuf, err);
			break;
		case Session::CLOSE:
#if CLOSE_ON_CLOSE
			return (-1);
#else
			return (0);
#endif
		default:
			break;
		}

#if DBG_SESS_NEXT
		sess_log_next(sess);
#endif
		switch (sess.nextAction())
		{
#if WITH_RETRY
		case Session::RETRY:
			WSCOL(WSL_CYAN);
			WSLOG(LVL_WARN, TGT_CONN | TGT_RETRY, "sess: RETRY");
			this->mod_evt(0);
			break;
#endif
		case Session::DOCGI:
			err = this->exec_cgi();
			if (err < 0)
			{
#if WITH_RETRY
				if (err == SYSCALL_ERR)
				{
					WSCOL(WSL_CYAN);
					WSLOG(LVL_WARN, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "retry", retry_cgi);
					this->serv.set_paused(); // failed : CGI
					retry_cgi++;
					this->mod_evt(0);
				}
#else
				this->set_err(500);
#endif
				return (0); // send error
			}
			this->res_cgi->push_body();
			break;
		case Session::WRSOCK:
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_CONN_RECV, "sess:  WRSOCK");
			this->req_cnt++;
			// expect no more input data ...
			// not true for UPLOAD => ERROR
#if CLOSE_ON_CLOSE
			this->mod_evt(-EPOLLIN);
#endif
			this->mod_evt(EPOLLOUT);
			break;
		case Session::RDSOCK:
			break;
		case Session::KPALIVE:
			WSCOL(WSL_PURPLE);
			WSLOG(LVL_DBG, TGT_KEEPA, "keep-alive (ip)");
			// no reset here (?)
			if (this->serv.get_paused())
				return (-1);
			return (0);
		case Session::CLOSE:
			WSCOL(WSL_RED);
			WSLOG(LVL_DBG, TGT_CONN_RECV, "sess:  CLOSE");
#if CLOSE_ON_CLOSE
			return (-1);
#else
			return (0);
#endif
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSCOL(WSL_RED);
		WSLOG(LVL_ERR, TGT_CONN, "ex: pollin\n", e.what());
		this->set_err(500); // CGI_ERR
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
#if DBG_SESS_NEXT
		sess_log_next(sess);
#endif
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
			case RSP_KPALIVE:
				WSCOL(WSL_PURPLE);
				WSLOG(LVL_DBG, TGT_KEEPA, "keep-alive (rsp) ", this->req_cnt);
				if (this->serv.get_paused())
					return (-1);
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
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "data");
			// WSLOG(LVL_DBG, TGT_CONN_SEND, "****\n", RESP);
			err = this->send(RESP);
		}
		else
		{
			switch (sess.nextAction())
			{
			case Session::CLOSE:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  CLOSE");
#if CLOSE_ON_CLOSE
				return (-1);
#else
				// this->mod_evt(EPOLLIN);
				this->mod_evt(-EPOLLOUT);
				return (0);
#endif
			case Session::KPALIVE:
				WSCOL(WSL_PURPLE);
				WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  KPALIVE");
				return (0);
			case Session::RDSOCK:
				WSLOG(LVL_DBG, TGT_CONN_SEND, "send:  RDSOCK");
				this->mod_evt(-EPOLLOUT);
				return (0);
			case Session::WRSOCK:
			default:
				std::string & RESP = sess.getResponse();
				if (RESP.size())
				{
					WSLOG(LVL_DBG, TGT_CONN_SEND, "send");
					WSLOG(LVL_DBG, TGT_CONN_SEND, "resp: " , RESP.size());
					// WSLOG(LVL_DBG, TGT_CONN_SEND, "data");
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
			// return (0); still need to check nextAction
		}
		WSLOG(LVL_DBG, TGT_CONN_SEND, "sent: ", err);

#if DBG_SESS_NEXT
		sess_log_next(sess);
#endif
		switch (sess.nextAction())
		{
		case Session::KPALIVE:
			WSCOL(WSL_PURPLE);
			WSLOG(LVL_DBG, TGT_KEEPA, "keep-alive (op) ", this->req_cnt);
			if (this->serv.get_paused())
				return (-1);
			this->reset();
			return (0);
		case Session::CLOSE:

			WSLOG(LVL_DBG, TGT_CONN_SEND, "sent:  CLOSE");
			// ATTN : upload
#if CLOSE_ON_CLOSE
			return (-1);
#else
			return (0);
#endif
		default:
			break;
		}
		return (err);
	}
	catch(const std::exception& e)
	{
		WSCOL(WSL_RED);
		WSLOG(LVL_ERR, TGT_CONN, "ex: pollout\n", e.what());
		this->set_err(500); // CGI_ERR
	}
	return (0);
}

int	Connection::rdhup(void)
{
	WSLOG(LVL_DBG, TGT_CONN, "RDHUP");
#if 0 // testing
	switch (sess.nextAction())
	{
	case Session::CLOSE:
		WSLOG(LVL_DBG, TGT_CONN, "- CLOSE");
		return (-1);
	case Session::RDSOCK:
		WSLOG(LVL_DBG, TGT_CONN, "- RDSOCK");
		return (-1);
	case Session::KPALIVE:
		WSLOG(LVL_DBG, TGT_CONN, "- KPALIVE");
		break;
	default:
		this->mod_evt(EPOLLOUT);
		break;
	}
	return (0);
#else
	this->mod_evt(EPOLLOUT);
	return (-1);
#endif
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

	int			err;

	CgiEnv *cgienv = new CgiEnv;
	err = cgienv->from_conn(*this);
	if (err < 0)
	{
		WSLOG(LVL_DBG, TGT_CGI, "cgienv: FAIL");
		delete (cgienv);
		return (this->set_err(500));
	}

	std::string &fcgi_sock = this->serv.get_conf().fcgi_sock;
	if (
		(cgienv->lang == CGI_PHP) &&
		!fcgi_sock.empty() &&
		sock_file(fcgi_sock.c_str())
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
			this->res_cgi->ka = this->sess.getRequest().keepalive(); //  && !retry_cgi;
			this->req_cnt++;
			return (err);
		}
		delete (cgienv);
		delete (fcgi);
		WSCOL(WSL_CYAN);
		WSLOG(LVL_WARN, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "fail ", retry_cgi);
		return(SYSCALL_ERR);
	}

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_CONN, "php :  pipe");

	cgi_pipes	pipes;

	if (pipes.init() < 0)
	{
		delete (cgienv);
		WSCOL(WSL_CYAN);
		WSLOG(LVL_WARN, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "fail ", retry_cgi);
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
		pipes.dup_err(); // cgi_err_log
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
		WSLOG(LVL_WARN, TGT_CONN | TGT_RETRY, "cgi : ", this->fd, "fail ", retry_cgi);
		return (SYSCALL_ERR);
	}
	this->res_cgi = pcgi;
	this->res_cgi->ka = this->sess.getRequest().keepalive(); //  && !retry_cgi;
	this->req_cnt++;
	return (err);
}
