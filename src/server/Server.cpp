/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 17:10:35 by kdonlon          ###   ########.fr       */
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
	paused(0)
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
		
	WSCOL(WSL_RED);
	WSLOG(LVL_TMP, TGT_SERV, "pause  ...");
	
	this->paused = 1;
	this->sfd_close();
	this->mod_evt(-EPOLLIN);
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
#if 0
		switch(errno)
		{
		case EMFILE:
			WSLOG(LVL_DBG, TGT_SERV, "EMFILE");
			break;
		case ENFILE:
			WSLOG(LVL_DBG, TGT_SERV, "ENFILE");
			break;
		default:
			break;
		}
#endif
		this->set_paused();
		// only close one .. what would the difference be (?)
		// or .. close TWO . leave one for the Connections "extra" fd 
		// but . it will probably get grabbed by the next accept
		// this->sfd_close();

		// or : do not try to re-accept immediately .. 
		// free (1) fd on each pause interval
		// but do not try to accept immediately  
		// which a cgi might use 
		
#if 0
		conn_fd = accept(this->fd, (struct sockaddr*) addr, &conn_asiz);
		if (conn_fd < 0)
		{
			acc_fail++;
			WSCOL(WSL_RED);
			WSLOG(LVL_ERR, TGT_SERV, "FAILED!");
		}
		else
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_ERR, TGT_SERV, "accepted!");
		}
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
	int			conn_fd;
	
	this->acc_cnt++;
	conn_fd = this->accept_conn();
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

	this->sfd_close(); // all of them (?!?)
	if (this->accept_conn() > 0)
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_ERR, TGT_SERV | TGT_TIMEO, "accepted!");
	}
	
	if (this->sfd_open() < 0)
	{
		WSCOL(WSL_PURPLE);
		WSLOG(LVL_TMP, TGT_SERV | TGT_TIMEO, "stay paused");
		this->ep->cli_info();
		return (false);
	}

	WSCOL(WSL_GREEN);
	WSLOG(LVL_TMP, TGT_SERV | TGT_TIMEO, "resume (!)");

	// tell all retry-ing conn to .. retry
	this->paused = 0;
	this->mod_evt(EPOLLIN);
	return (false);
}

unsigned short	Server::get_port(void)	const
{
	return (this->port);
}
