/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:21:04 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/20 22:17:41 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# ifndef DBG_CANON
#  define DBG_CANON 0
# endif

# include <iostream>
# include <unistd.h>		/* close(fd) */
# include <arpa/inet.h>		/* struct sockaddr_in */
# include <fcntl.h>

# include "Epoll.hpp"
# include <vector>

class Connection;

class Server : public EpollClient
{
public:
	Epoll		 		&ep;
	
	// Server (void); // Q: empty reference (?)
	Server (Epoll & epoll, unsigned short p);
	Server (const Server & that);
	Server & operator = (const Server & that);
	~Server();

	int		init(void);
	int		get_fd(void);
	int		pollin(void);
	int		pollout(void);
	
private:
	int					fd;
	struct sockaddr_in	addr;
	unsigned short		port;

	std::vector<Connection *> conn;
	// std::vector<Route> route;

};

std::ostream& operator << (std::ostream & os, Server & obj);

#endif

