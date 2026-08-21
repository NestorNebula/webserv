/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 20:48:09 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>
# include "Socket.hpp"
# include <iostream>
# include <sys/wait.h>
# include <cstring>
# include <algorithm>

# include "Socket.hpp"
# include "Epoll.hpp"
# include "EpollClient.hpp"
# include "CgiEnv.hpp"

# include "Session.hpp"

class Server;
class CgiPipe;
class ResourceCgi;

class Connection : public EpollClient
{
private:
	Connection				(const Connection & that);
	Connection & operator = (const Connection & ) 
		{ return (*this); }
		
public:
	Connection (Epoll *_ep, int _fd, Server &_serv);
	~Connection();
	
	ssize_t			pollin (void);
	ssize_t			pollout(void);
	int				rdhup  (void);
	int				hup    (void);
	bool			timeo  (time_t now);
	
	int				set_err(int e);
	
	void			set_addr(struct sockaddr_in *a);
	std::string		&get_addr(void);
	
	Session			sess;
	Server			&serv;
	
	void			cgi_rem(EpollClient *epc);
	
private:
	void			reset(void);
	
	ResourceCgi			*res_cgi;
	int					exec_cgi(void);
	
	struct sockaddr_in	addr;
	std::string			astr;
	
	int					req_cnt;
};

#endif

