/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/18 18:53:59 by kdonlon          ###   ########.fr       */
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

# ifndef CONN_TIMEOUT
#  define CONN_TIMEOUT 60
# endif

class Connection : public EpollClient
{
private:
// DEMO
	Connection				(const Connection & that); // : EpollClient(that), 
		// sess(that.serv.get_conf()), serv(that.serv), req_cnt(0) {}
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
	
	void			set_err(int e);
	
	void			set_addr(struct sockaddr_in *a);
	std::string		&get_addr(void);
	
	Session			sess;
	Server			&serv;
	int				req_body_status(void);
	
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

