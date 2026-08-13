/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 20:07:03 by kdonlon          ###   ########.fr       */
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

# include "bridge.hpp"

class br_Server;
class CgiPipe;
class ResourceCgi;

# ifndef CONN_TIMEOUT
#  define CONN_TIMEOUT 10
# endif

class Connection : public EpollClient
{
private:
	Connection				(const Connection & that) : EpollClient(that), 
		serv(that.serv), req_cnt(0) {}
	Connection & operator = (const Connection & ) 
		{ return (*this); }
		
public:
	Connection (Epoll *_ep, int _fd, br_Server &_serv);
	~Connection();
	
	ssize_t			pollin (void);
	ssize_t			pollout(void);
	int				rdhup  (void);
	int				hup    (void);
	bool			timeo  (time_t now);
	
	void			set_err(int e);
	
	void			set_addr(struct sockaddr_in *a);
	std::string		&get_addr(void);
	
// WEBSERV / SESSION
	br_Session		sess;
	br_Server		&serv;
	int				req_body_status(void);
	
	void			cgi_rem(EpollClient *epc);
	
private:
	void			reset(void);
	
	ResourceCgi			*res_cgi;
	int					exec_cgi(void);
	
	int					send_error(void);
	std::string			estr;
	
	struct sockaddr_in	addr;
	std::string			astr;
	
	int					req_cnt;
};

#endif

