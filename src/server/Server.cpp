/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 14:42:46 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"

br_Server::br_Server (Epoll *_ep, unsigned short p) : 
	EpollClient(_ep, EPC_SERV, -1), 
	port(p),
	acc_cnt(0)
{
	this->addr.sin_family		= AF_INET;
	this->addr.sin_addr.s_addr	= INADDR_ANY;
	this->addr.sin_port			= htons(this->port);
	if (this->init() < 0)
		throw (std::runtime_error("Server : construct failed"));

	// proj_root = std::string("/home/kdonlon/Documents/Projects/webserv/git/");
	proj_root = std::string("/media/kdonlon/data/Documents/42/webserv/git/");
	data_root = proj_root + std::string("tst/server/");
	fcgi_sock = data_root + std::string("FCGI/.php-fpm/SOCK");
	pycgi = proj_root + std::string("pycgi/");
	bin_php = std::string("/usr/bin/php-cgi");
	bin_py = std::string("/usr/bin/python3");
	bin_pl = std::string("/usr/bin/perl");
};

br_Server::~br_Server()
{
	WsLog::_(LVL_DBG, TGT_SERV, " (~) Server");
	WsLog::_(LVL_DBG, TGT_SERV, "accepted: ", acc_cnt);
};

int br_Server::init(void)
{
	int	err;

	if (this->port == 0)
	{
		WsLog::_(LVL_ERR, TGT_SERV, "bad port");
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

	WsLog::_(LVL_INFO, TGT_SERV, "listening on port: ", this->port);
	return (err);
}

ssize_t	br_Server::pollin(void)
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

ssize_t	br_Server::pollout(void)
{
	return (0);
}

int	br_Server::rdhup(void) 
{
	return (0);
}

int	br_Server::hup(void) 
{
	return (0);
}

bool	br_Server::timeo  (time_t)
{
	return (false);
}

unsigned short	br_Server::get_port(void)	const
{
	return (this->port);
}
