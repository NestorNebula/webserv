/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:10 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/20 22:38:35 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Connection.hpp"
#include "Socket.hpp"

// Server::Server (void) : fd (-1), port(0)
// {
// };

Server::Server (Epoll & epoll, unsigned short p) : ep(epoll), fd (-1), port(p)
{
	this->addr.sin_family = AF_INET;
	this->addr.sin_addr.s_addr = INADDR_ANY;
	this->addr.sin_port = htons(this->port);
};

Server::Server(const Server & that) : ep(that.ep)
{
	(void) that;
}

Server & Server::operator = (const Server & that)
{
	if (this == &that)
		return (*this);
	// FREE DATA : this
	// COPY DATA : this->val = that.val
	return (*this);
}

Server::~Server()
{
	if (this->fd != -1)
		close(this->fd);
	for (size_t k=0; k < this->conn.size(); k++)
		delete (this->conn[k]);
};

std::ostream& operator << (std::ostream & os, Server & obj)
{
	(void)obj;
	return (os);
}

int Server::get_fd(void)
{
	return (this->fd);
}

int Server::init(void)
{
	int	err;

	if (this->port == 0)
		return (-1); // port not set 
	
	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->fd < 0)
	{
		std::cerr << "serv : failed socket\n";
		return (-1);
	}
	const int reuse = 1;
	err = setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (err < 0)
	{
		std::cerr << "serv : failed reuse\n";
		return (err);
	}
					
	err = bind(this->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (err < 0)
	{
		std::cerr << "serv : failed bind\n";
		return (err);
	}

	err = sock_non_block(this->fd);
	
	if (err < 0)
	{
		std::cerr << "serv : failed fcntl\n";
		return (err);
	}
	
// The backlog argument defines the maximum length to which the queue
// of pending connections for sockfd may grow.  If a connection
// request arrives when the queue is full, the client may receive an
// error with an indication of ECONNREFUSED or, if the underlying
// protocol supports retransmission, the request may be ignored so
// that a later reattempt at connection succeeds.			
	err = listen(this->fd, 4);
	if (err < 0)
	{
		std::cerr << "serv : failed listen\n";
		return (err);
	}
	return (0);
}

int	Server::pollin(void)
{
	int	err;

	int conn_fd = accept(this->fd, NULL, NULL); // peer_addr
	if (conn_fd < 0)
	{
		std::cerr << "serv : failed accept\n";
		return (conn_fd);
	}
	err = sock_non_block(conn_fd);
	if (err < 0)
		return (err);

	Connection *c = new Connection(conn_fd, this);
	
	this->ep.add(conn_fd, EPOLLIN, c);

	this->conn.push_back(c);
	return (err);
}

int	Server::pollout(void)
{
	int	err = 0;
		
	// must write something .. yes (?)
	return (err);
}
