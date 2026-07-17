/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/17 11:39:04 by kdonlon          ###   ########.fr       */
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

# include "Epoll.hpp"
# include "EpollClient.hpp"
# include "CgiEnv.hpp"

# include "bridge.hpp"
// # include <map>



// CONN_HEAD_COMPLETE
// CONN_BODY_COMPLETE

// RSRC_SENT_HEAD
// RSRC_COMPLETE -- check error

#define CONN_HAS_HEAD 1
#define CONN_HAS_RSRC 2
#define RSRC_HAS_RESP 3
#define CONN_SENT_RESP 4
#define RSRC_ERROR 6

class Server;
class CgiPipe;


class ResourceCgi
{
public:
	ResourceCgi(void) : pid(0), ip(NULL), op(NULL), stat(-1) {}
	~ResourceCgi();
	
	pid_t			pid;
	CgiPipe			*ip;
	CgiPipe			*op;
	int				stat;

	int				status(int opt);
	void			rem(CgiPipe *epc);
	
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
	int				hup    (void) { return (0); }
	bool			timeo  (time_t now);
	
	void			set_err(int e);
	void			set_addr(struct sockaddr_in *a) { this->addr = *a; }

	Session			sess;
	std::string		resp;
	
// Session/Resource
	int				cgi_inp(void);
	int				cgi_out(const char *buf, ssize_t siz);
	

	ResourceCgi		cgi;
// 	int				cgi_status(void);
// 	void			rem_cgi(CgiPipe *epc);
// // Resource
// 	pid_t			cgi_pid;
// 	CgiPipe			*cgi_ip;
// 	CgiPipe			*cgi_op;
	


public:
	Server			&serv;

private:
	int				exec_cgi(void);
	
public:
	struct sockaddr_in	addr;
private:
	int				req_cnt;
public:
	int				state;
};

#endif

