/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 15:29:33 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>
# include "Epoll.hpp"
# include <iostream>
# include <sys/wait.h>
# include <cstring>

# ifndef CONN_BUF_SIZ
#  define CONN_BUF_SIZ 4096
# endif


# ifndef CONN_OUT_SIZ
#  define CONN_OUT_SIZ 4096
# endif


class Server;
class CgiPipe;


class Connection : public EpollClient
{
public:
	Server			&serv;
	
	Connection (int _fd, Server &_serv);
	Connection (const Connection & that);
	Connection & operator = (const Connection & that);
	~Connection();
	
	int		pollin(void);
	int		pollout(void);
	
private:
	time_t			lact;
	int				timeout(void); // EpollClient - called on pollin, pollout
	int				shutdown(void);

	int				exec_cgi(void);
	
	CgiPipe			*cgi_in;
	CgiPipe			*cgi_out;
	int				req_cnt;
};

std::ostream& operator << (std::ostream & os, Connection & obj);

class CgiPipe : public EpollClient
{
private:
	CgiPipe (const CgiPipe & that) : EpollClient(EPC_CGI, that.fd), conn(that.conn) {};
	CgiPipe & operator = (const CgiPipe & that) { return (*this); }
public:
	CgiPipe (int _fd, Connection & _conn);
	~CgiPipe() {}
	
	int		pollin(void);
	int		pollout(void);

private:
	Connection	&conn;
};

// CgiPipe
	// pollin
		// read from STDOUT of cgi-script
			// TO : Connection::obuf (?)
	// pollout
		// write to STDIN   of cgi-script
			// FROM : Connection::ibuf (?)
			
// fuck : every pipefd going through epoll (?)
	// cgi-in : can read : write from ibuf
	// cgi-in : can write .. not really : dup'ed
	// cgi-out : can read .. dup'ed
	// cgi-out : can write
		
// Connection *conn;

#endif

