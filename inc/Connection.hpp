/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/19 10:03:54 by kdonlon          ###   ########.fr       */
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

class Server;
class CgiPipe;

class ResourceCgi
{
public:
	ResourceCgi(void) : pid(0), ip(NULL), op(NULL), stat(-1), hed(0) {}
	~ResourceCgi();
	
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
	int			stat;
	int			hed;

	int			status(int opt);
	void		rem(CgiPipe *epc);
	void		reset(void);
};

class Connection : public EpollClient
{
private:
	Connection				(const Connection & that) : EpollClient(that), 
		serv(that.serv), req_cnt(0) {}
	Connection & operator = (const Connection & ) 
		{ return (*this); }
		
public:
	Connection (Epoll *_ep, int _fd, Server &_serv);
	~Connection();
	
	ssize_t			pollin (void);
	ssize_t			pollout(void);
	int				hup    (void);
	bool			timeo  (time_t now);
	
	void			set_err(int e);
	void			set_addr(struct sockaddr_in *a);
	std::string		&get_addr(void);
	
	Session			sess;
	
// Session/Resource
	ResourceCgi		cgi;
	int				req_body_status(void);
	int				cgi_data(const char *buf, ssize_t siz);
	
	// only CgiEnv::server_port
	Server			&serv;

private:
	int					exec_cgi(void);
	std::string			ostr;
	
	struct sockaddr_in	addr;
	std::string			astr;
	
	int					req_cnt;
};

#endif

