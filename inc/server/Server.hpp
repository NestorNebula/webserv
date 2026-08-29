/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:04 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/29 21:40:16 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <iostream>
# include <string>
# include <unistd.h>
# include <arpa/inet.h>	
# include <fcntl.h>

# include "Epoll.hpp"
# include "EpollClient.hpp"

# include "ServerConfig.hpp"

	// The backlog argument defines the maximum length to which the queue
	// of pending connections for sockfd may grow.  If a connection
	// request arrives when the queue is full, the client may receive an
	// error with an indication of ECONNREFUSED or, if the underlying
	// protocol supports retransmission, the request may be ignored so
	// that a later reattempt at connection succeeds.
	
# ifndef SERV_BACKLOG
#  define SERV_BACKLOG 128
# endif

class Server : public EpollClient
{
private:
	Server				(const Server & that) : EpollClient(that) {}
	Server & operator =	(const Server & ) { return (*this); }

public:
	Server (Epoll *_ep, unsigned short p, const ServerConfig &_conf);
	~Server();

	ssize_t				pollin (void);
	ssize_t				pollout(void);
	int					rdhup  (void);
	int					hup    (void);
	bool				timeo  (time_t);
	
	unsigned short		get_port(void)	const;
	ServerConfig		&get_conf() { return (this->conf); }
	
private:
	ServerConfig		conf;
	struct sockaddr_in	addr;
	unsigned short		port;
	
	int					init(void);
	int					acc_cnt;
};

#endif

