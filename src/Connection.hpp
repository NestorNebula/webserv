/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/20 22:11:19 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# ifndef DBG_CANON
#  define DBG_CANON 0
# endif

# include <unistd.h>
# include "Epoll.hpp"
# include <iostream>

class Server;

class Connection : public EpollClient
{
public:
	Connection (void);
	Connection (int _fd, Server *_serv);
	Connection (const Connection & that);
	Connection & operator = (const Connection & that);
	~Connection();
	
	int		get_fd(void);
	int		pollin(void);
	int		pollout(void);
	
private:
	int			fd;
	Server		*serv;
	// Epoll .. to MOD when wanting to check (write) availability
	std::string	ibuf;
	std::string	obuf;
};

std::ostream& operator << (std::ostream & os, Connection & obj);

#endif

