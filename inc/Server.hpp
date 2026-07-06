/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:04 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 15:13:40 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <unistd.h>		/* close(fd) */
# include <arpa/inet.h>		/* struct sockaddr_in */
# include <fcntl.h>

# include "Epoll.hpp"
# include "EpollClient.hpp"

# ifndef DBG_SERV
#  define DBG_SERV 0
# endif

	// The backlog argument defines the maximum length to which the queue
	// of pending connections for sockfd may grow.  If a connection
	// request arrives when the queue is full, the client may receive an
	// error with an indication of ECONNREFUSED or, if the underlying
	// protocol supports retransmission, the request may be ignored so
	// that a later reattempt at connection succeeds.	
# ifndef SERV_BACKLOG
#  define SERV_BACKLOG 256
# endif


// env .. map .. or fixed "key=val"
class Connection;

class Server : public EpollClient
{
private:
	Server (const Server & that) : 
		EpollClient(that) {}
	Server & operator = (const Server & )
		{ return (*this); }

public:
	Server (Epoll & _ep, unsigned short p);
	~Server();

	int				pollin(void);
	int				pollout(void);
	
	unsigned short	get_port(void) const { return (this->port); }
private:
	struct sockaddr_in	addr;
	unsigned short		port;
	
	int					init(void);
	// std::vector<Route> route;
};

#endif

