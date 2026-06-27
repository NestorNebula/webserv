/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/27 22:21:47 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"

Server::Server (Epoll & epoll, unsigned short p) : EpollClient(EPC_SERV, -1), ep(epoll), port(p)
{
	this->addr.sin_family = AF_INET;
	this->addr.sin_addr.s_addr = INADDR_ANY;
	this->addr.sin_port = htons(this->port);
	if (this->init())
		throw (std::runtime_error("Server : construct failed"));
};

Server::Server(const Server & that) : EpollClient(EPC_SERV, that.fd), ep(that.ep)
{
	(void) that;
}

Server & Server::operator = (const Server & that)
{
	if (this == &that)
		return (*this);
	return (*this);
}

Server::~Server()
{
	this->ep.del(this); // (ep) MUST STILL EXIST
	// if (this->fd != -1)
	// 	close(this->fd);
};

std::ostream& operator << (std::ostream & os, Server & obj)
{
	(void)obj;
	return (os);
}

int Server::init(void)
{
	int	err;

	if (this->port == 0)
	{
		std::cerr << "serv : bad port\n";
		return (-1);
	}
	
	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->fd < 0)
	{
		std::cerr << "serv : failed socket\n"; // strerror
		// strerror
		return (-1);
	}
	const int reuse = 1;
	err = setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (err < 0)
	{
		std::cerr << "serv : failed reuse\n"; // strerror
		return (err);
	}
					
	err = bind(this->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (err < 0)
	{
		std::cerr << "serv : failed bind\n"; // strerror
		return (err);
	}

	err = sock_non_block(this->fd);
	if (err < 0)
	{
		std::cerr << "serv : failed fcntl\n"; // strerror
		return (err);
	}
	
	err = listen(this->fd, SERV_BACKLOG); 
	if (err < 0)
	{
		std::cerr << "serv : failed listen\n"; // strerror
		return (err);
	}

	err = this->ep.add(this, EPOLLIN);
	return (err);
}

int	Server::pollin(void)
{
	int	err;

	int conn_fd = accept(this->fd, NULL, NULL); // peer_addr 
	if (conn_fd < 0)
	{
		std::cerr << "serv : failed accept\n"; // strerror
		return (conn_fd);
	}
	err = sock_non_block(conn_fd);
	if (err < 0)
	{
		return (err);
	}
	Connection *c = new Connection(conn_fd, *this);
	
	err = this->ep.add(c, EPOLLIN);
#if DBG_SERV
	std::cerr << "serv  : accept " << conn_fd << std::endl;
#endif
	return (err);
}

int	Server::pollout(void)
{
	int	err = 0;
		
		// When would we ever write to the Server's (fd)
	return (err);
}
