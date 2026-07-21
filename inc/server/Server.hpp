/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:04 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/20 12:09:16 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <unistd.h>
# include <arpa/inet.h>	
# include <fcntl.h>

# include "Epoll.hpp"
# include "EpollClient.hpp"

	// The backlog argument defines the maximum length to which the queue
	// of pending connections for sockfd may grow.  If a connection
	// request arrives when the queue is full, the client may receive an
	// error with an indication of ECONNREFUSED or, if the underlying
	// protocol supports retransmission, the request may be ignored so
	// that a later reattempt at connection succeeds.
	
# ifndef SERV_BACKLOG
#  define SERV_BACKLOG 256
# endif

class Connection;

class Server : public EpollClient
{
private:
	Server				(const Server & that) : EpollClient(that) {}
	Server & operator =	(const Server & ) { return (*this); }

public:
	Server (Epoll *_ep, unsigned short p);
	~Server();

	ssize_t				pollin (void);
	ssize_t				pollout(void);
	int					hup    (void);
	bool				timeo  (time_t);
	
	unsigned short		get_port(void)	const;
	
private:
	struct sockaddr_in	addr;
	unsigned short		port;
	
	int					init(void);
	int					acc_cnt;
	// std::vector<Route> route;
};

#endif

