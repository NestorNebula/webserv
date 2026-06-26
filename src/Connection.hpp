/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 21:48:45 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>
# include "Epoll.hpp"
# include <iostream>
# include <sys/wait.h>


# ifndef DBG_CONN_READ
#  define DBG_CONN_READ 0
# endif

# ifndef DBG_CONN_WRITE
#  define DBG_CONN_WRITE 0
# endif


# ifndef CONN_BUF_SIZ
#  define CONN_BUF_SIZ 4096
# endif


# ifndef CONN_OUT_SIZ
#  define CONN_OUT_SIZ 4096
# endif


class Server;

typedef enum
{
	CONN_STATE_INIT,
	CONN_STATE_SHUTDOWN,
	CONN_STATE_ERROR
}	e_conn_state;

class Connection : public EpollClient
{
public:
	Connection (int _fd, Server &_serv);
	Connection (const Connection & that);
	Connection & operator = (const Connection & that);
	~Connection();
	
	int		pollin(void);
	int		pollout(void);
	
	e_conn_state	get_state(void) const { return (this->state); }
	
private:
	Server			&serv;
	std::string		ibuf;
	std::string		obuf;
	time_t			lact;
	int				timeout(void);
	int				shutdown(void);

	int				recv(void);
	int				send(std::string & buf);
	e_conn_state	state;
	int				error; // HTTP
	int				req_cnt;
};

std::ostream& operator << (std::ostream & os, Connection & obj);


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

