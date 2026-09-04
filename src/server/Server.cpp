/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/04 15:50:24 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"

Server::Server (Epoll *_ep, unsigned short p, const ServerConfig &_conf) : 
	EpollClient(_ep, EPC_SERV, -1), 
	conf(_conf),
	port(p),
	acc_cnt(0),
	acc_err(0),
	acc_fail(0),
	paused(0),
	freed_fd(0)
{
	this->addr.sin_family		= AF_INET;
	this->addr.sin_addr.s_addr	= INADDR_ANY;
	this->addr.sin_port			= htons(this->port);
	if (this->init() < 0)
		throw (std::runtime_error("Server : construct failed"));
};

Server::~Server()
{
	WSLOG(LVL_TMP, TGT_SERV, " (~) Server");
	WSLOG(LVL_TMP, TGT_SERV, "acc cnt : ", acc_cnt);
	WSLOG(LVL_TMP, TGT_SERV, "acc err : ", acc_err);
	WSLOG(LVL_TMP, TGT_SERV, "acc fail: ", acc_fail);
	this->sfd_close();
};

int Server::init(void)
{
	int	err;

	if (this->port == 0)
	{
		WSLOG(LVL_ERR, TGT_SERV, "bad port");
		return (-1);
	}

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

	int	nconn = this->ep->cli_cnt(EPC_CONN);
	WSCOL(WSL_RED);
	WSLOG(LVL_TMP, TGT_SERV, "pause  ...  ", this->port);
	WSLOG(LVL_TMP, TGT_SERV, "nconn  ...  ", nconn);
#if 0 // FREED_FD
	this->freed_fd = this->ep->cli_cnt(EPC_CONN);

	WSCOL(WSL_RED);
	WSLOG(LVL_TMP, TGT_SERV, "pause  ... ", this->freed_fd);
	
	if (this->freed_fd > 6)
		this->freed_fd = 6;
#endif
	// the idea : cede these to a CGI that needs to get started
	this->sfd_close();
	this->mod_evt(-EPOLLIN);
}

void	Server::conn_closed(void)
{
	if (!this->paused)
		return;
	this->freed_fd++;

	if (this->freed_fd > 4)
		this->lact = this->lact - SERV_PAUSE;
#if 0 // FREED_FD

	this->freed_fd--;
	WSCOL(WSL_PURPLE);
	WSLOG(LVL_TMP, TGT_SERV, "close  ... ", this->freed_fd);
	if (this->freed_fd <= 0)
	{
		this->lact = this->lact - SERV_PAUSE;
	}
#endif
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
		this->set_paused(); // failed : accept()
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

#if 0 // FREED_FD
	if (this->freed_fd > 0)
	{
		WSCOL(WSL_PURPLE);
		WSLOG(LVL_TMP, TGT_SERV | TGT_TIMEO, "freed ", this->freed_fd);
		this->ep->cli_info();
		return (false);
	}
#endif
	this->sfd_close();
	// if (this->accept_conn() > 0)
	// {
	// 	WSCOL(WSL_GREEN);
	// 	WSLOG(LVL_ERR, TGT_SERV | TGT_TIMEO, "accepted!");
	// }
	if (this->sfd_open() < 0)
	{
		// unable to open all spare_fds
		WSCOL(WSL_PURPLE);
		WSLOG(LVL_TMP, TGT_SERV | TGT_TIMEO, "stay paused ", this->port);
		this->ep->cli_info();
		return (false);
	}
	// no guarantee they'll get started .. before we get the next one 
	if (this->accept_conn() > 0)
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_ERR, TGT_SERV | TGT_TIMEO, "accepted! ", this->port);
	}
	// free (3) for CGI .. 

	WSCOL(WSL_GREEN);
	WSLOG(LVL_TMP, TGT_SERV | TGT_TIMEO, "resume (!) ", this->port);

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
			WSLOG(LVL_TMP, TGT_SERV, "sfd fail: ", i);
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

