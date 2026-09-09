/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/09 18:56:10 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"
#include "helpers.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>

static int uaddr_set(void * addr, const char * str = NULL, unsigned short p = 0)
{
	int	err;

	std::string prt = toString(p);

	struct addrinfo hint;
    std::memset(&hint, '\0', sizeof hint);

    hint.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = PF_INET;
	hint.ai_protocol = IPPROTO_TCP;

	struct addrinfo * res = NULL;
    err = getaddrinfo(str, prt.c_str(), &hint, &res);
    if (err)
    {
		WSLOG(LVL_TMP, TGT_SERV, "addr : ", str);
		WSLOG(LVL_TMP, TGT_SERV, "addr : ", gai_strerror(err));
		freeaddrinfo(res);
		return (-1);
    }
    struct addrinfo * chk = res;
#if 0
    while (chk)
    {
		std::cerr << "\tflags: " << chk->ai_flags << "\tfamily: " << chk->ai_family << "\tsocktype: " << chk->ai_socktype << "\tprotocol: " << chk->ai_protocol << std::endl;

		struct sockaddr_in * inaddr = (struct sockaddr_in *) chk->ai_addr;
		std::cerr << inaddr->sin_addr.s_addr << std::endl;
		std::cerr << ntohs(inaddr->sin_port) << std::endl;

    	chk = chk->ai_next;
    }
    chk = res;
#endif
    while (chk)
    {
    	// if (chk->ai_family == fam)
    	{
		   std::memcpy(addr, chk->ai_addr, chk->ai_addrlen);
		   freeaddrinfo(res);
		   return (0);
    	}
    	chk = chk->ai_next;
    }

	freeaddrinfo(res);
	return (-1);
}


Server::Server (Epoll *_ep, const ServerConfig &_conf) :
	EpollClient(_ep, EPC_SERV, -1),
	conf(_conf),
	port(_conf.port),
	acc_cnt(0),
	acc_err(0),
	acc_fail(0),
	paused(0),
	freed_fd(0)
{
	if (this->init() < 0)
	{
		WSLOG(LVL_ERR, TGT_SERV, "host : ", conf.host);
		WSLOG(LVL_ERR, TGT_SERV, "port :", conf.port);
		throw (std::runtime_error("serv  : construct failed"));
	}
};

Server::~Server()
{
	WSLOG(LVL_DBG, TGT_SERV, " (~) Server");
	WSLOG(LVL_DBG, TGT_SERV, "acc cnt : ", acc_cnt);
	WSLOG(LVL_DBG, TGT_SERV, "acc err : ", acc_err);
	WSLOG(LVL_DBG, TGT_SERV, "acc fail: ", acc_fail);
	this->sfd_close();
};

int Server::init(void)
{
	int	err;

	if (uaddr_set(&this->addr, conf.host.c_str(), conf.port) < 0)
		return (-1);

	if (this->sfd_open() < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "spare_fd"));

	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->fd < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "socket"));

	const int reuse = 1;
	err = setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (err < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "setsockopt"));

	err = bind(this->fd, (struct sockaddr *) &addr, sizeof(addr));
	if (err < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "bind"));

	err = sock_non_block(this->fd);
	if (err < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "sock non-block"));

	err = listen(this->fd, SERV_BACKLOG);
	if (err < 0)
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "listen"));

	err = this->ini_evt(EPOLLIN);

	WsLog::_(LVL_MAIN, TGT_SERV, "listening on port: ", this->port);
	return (err);
}

void	Server::set_paused(void)
{
	if (this->paused)
		return;

	this->paused = 1;

	WSCOL(WSL_RED);
	WSLOG(LVL_DBG, TGT_RETRY, this->port, "pause  ...  ");
	// WSLOG(LVL_TMP, TGT_SERV, "nconn  ...  ", this->ep->cli_cnt(EPC_CONN));

	this->sfd_close();
	this->mod_evt(-EPOLLIN);
}

void	Server::conn_closed(void)
{
	if (!this->paused)
		return;
	this->freed_fd++;

	if (this->freed_fd > 6)
		this->lact = this->lact - SERV_PAUSE;
}

int	Server::accept_conn(void)
{
	int					conn_fd;
	struct sockaddr_in	conn_addr;
	socklen_t			conn_asiz = sizeof(struct sockaddr_in);

	conn_fd = accept(this->fd, (struct sockaddr*) &conn_addr, &conn_asiz);
	if (conn_fd < 0)
	{
		acc_err++;
		WsLog::_errno(LVL_ERR, TGT_SERV, "accept");
#if WITH_RETRY
		this->set_paused(); // failed : accept()
#endif
		return (0);
	}

	int err = sock_non_block(conn_fd);
	if (err < 0)
	{
		close(conn_fd);
		WsLog::_errno(LVL_ERR, TGT_SERV, "sock non-block");
		return (0);
	}

	Connection *c = new Connection(this->ep, conn_fd, *this);

	err = c->ini_evt(EPOLLIN);
	if (err < 0)
	{
		delete (c);
		return (0);
	}
	c->set_addr(&conn_addr);
	return (conn_fd);
}

ssize_t	Server::pollin(void)
{
	this->acc_cnt++;

	int conn_fd = this->accept_conn();
	if (conn_fd < 0)
		return (0);
	return (0);
}

ssize_t	Server::pollout(void)
{
	return (0);
}

int	Server::rdhup(void)
{
	return (0);
	// this->ep->cli_info();
}

int	Server::hup(void)
{
	return (0);
}

bool	Server::timeo  (WsTime & now)
{
	// if (this->lact.not_set())
	// 	return (false);
	// if (this->lact.after(now))
	// 	return (false);

	if (!this->paused)
		return (false);
	if ((this->lact + SERV_PAUSE).after(now))
		return (false);

	this->lact = now;

	this->sfd_close();
	if (this->sfd_open() < 0)
	{
		WSCOL(WSL_PURPLE);
		WSLOG(LVL_DBG, TGT_RETRY, this->port, "stay paused");
		this->ep->cli_info();
		return (false);
	}
	if (this->accept_conn() > 0)
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_DBG, TGT_RETRY, this->port, "accepted!");
	}
	// free (3) for CGI ..

	WSCOL(WSL_GREEN);
	WSLOG(LVL_DBG, TGT_RETRY, this->port, "resume (!)");

	this->freed_fd = 0;
	this->paused = 0;
	this->mod_evt(EPOLLIN);

	return (false);
}

unsigned short	Server::get_port(void)	const
{
	return (this->port);
}

int	Server::sfd_open(void)
{
	for (int i=0; i < SPARE_FD; i++)
	{
		this->spare_fd[i] = open("/dev/null", O_RDONLY);
		if (this->spare_fd[i] < 0)
		{
			WSLOG(LVL_DBG, TGT_SERV, "sfd fail: ", i);
			sfd_close();
			return (-1);
			// return (i > 0) ? (0) : (-1);
		}
	}
	return (0);
}

void	Server::sfd_close(void)
{
	for (int i=0; i < SPARE_FD; i++)
	{
		fd_close(this->spare_fd + i);
	}
}

