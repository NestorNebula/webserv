/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/20 12:10:54 by kdonlon          ###   ########.fr       */
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
private:
	ResourceCgi		cgi;
	
public:
	int				req_body_status(void);
	int				cgi_data(const char *buf, ssize_t siz);
	void			cgi_rem(CgiPipe *epc);
	int				cgi_status(int opt);
	
	Server			&serv;

private:
	int					exec_cgi(void);
	std::string			ostr;
	
	struct sockaddr_in	addr;
	std::string			astr;
	
	int					req_cnt;
};

#endif

