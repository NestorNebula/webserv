/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 09:41:11 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"

Server::Server (Epoll *_ep, unsigned short p, const ServerConfig &_conf) : 
	EpollClient(_ep, EPC_SERV, -1), 
	conf(_conf),
	port(p),
	acc_cnt(0)
{
	this->addr.sin_family		= AF_INET;
	this->addr.sin_addr.s_addr	= INADDR_ANY;
	this->addr.sin_port			= htons(this->port);
	if (this->init() < 0)
		throw (std::runtime_error("Server : construct failed"));
// #kd
// UGLY:
	// std::string git_root = conf.root + std::string("../");
	// fcgi_sock = git_root + std::string("tst/server/FCGI/.php-fpm/SOCK");

	// pycgi = git_root + std::string("pycgi/");
};

Server::~Server()
{
	WSLOG(LVL_DBG, TGT_SERV, " (~) Server");
	WSLOG(LVL_DBG, TGT_SERV, "accepted: ", acc_cnt);
};

int Server::init(void)
{
	int	err;

	if (this->port == 0)
	{
		WSLOG(LVL_ERR, TGT_SERV, "bad port");
		return (-1);
	}
	
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

	WSLOG(LVL_INFO, TGT_SERV, "listening on port: ", this->port);
	return (err);
}

ssize_t	Server::pollin(void)
{
	ssize_t				err;
	struct sockaddr_in	conn_addr;
	socklen_t			conn_asiz = sizeof(conn_addr);
	int					conn_fd;

	conn_fd = accept(this->fd, (struct sockaddr*) &conn_addr, &conn_asiz);
	if (conn_fd < 0)
		return (WsLog::_errno(LVL_DBG, TGT_SERV, "accept"));
		
	err = sock_non_block(conn_fd);
	if (err < 0)
	{
		close(conn_fd);
		return (WsLog::_errno(LVL_ERR, TGT_SERV, "sock non-block"));
	}
	
	Connection *c = new Connection(this->ep, conn_fd, *this);
	
	err = c->ini_evt(EPOLLIN);
	if (err < 0)
	{
		delete (c);
		return (err);
	}
	c->set_addr(&conn_addr);

	this->acc_cnt++;
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

bool	Server::timeo  (time_t)
{
	return (false);
}

unsigned short	Server::get_port(void)	const
{
	return (this->port);
}
