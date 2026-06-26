/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 22:55:48 by kdonlon          ###   ########.fr       */
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
class CgiPipes;

typedef enum
{
	CONN_STATE_INIT,
	CONN_STATE_SHUTDOWN,
	CONN_STATE_ERROR
}	e_conn_state;

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
	
	e_conn_state	get_state(void) const { return (this->state); }
	
private:
	std::string		ibuf;
	std::string		obuf;
	time_t			lact;
	int				timeout(void);
	int				shutdown(void);

	CgiPipes		*cgi;
	int				recv(void);
	int				send(std::string & buf);
	e_conn_state	state;
	int				error; // HTTP
	int				req_cnt;
};

std::ostream& operator << (std::ostream & os, Connection & obj);

class CgiPipes : public EpollClient
{
private:
	CgiPipes (const CgiPipes & that) : EpollClient(EPC_CGI, that.fd), conn(that.conn) {};
	CgiPipes & operator = (const CgiPipes & that) { return (*this); }
public:
	CgiPipes (Connection & _conn) : EpollClient(EPC_CGI, -1), conn(_conn)
	{
		int	err;

		err = this->init();
		if (err < 0)
			throw (std::runtime_error("Cgi : bad create"));
	}
	~CgiPipes() {}
	
	int		pollin(void);
	int		pollout(void);

	int		ifd(void) const { return (this->p1[1]); }
	int		ofd(void) const { return (this->p2[0]); }
private:
	Connection	&conn;
	int			p1[2];
	int			p2[2];
	int			init(void)
	{
		int	err;

		err = pipe(p1);
		if (err < 0)
			return (err);
		err = pipe(p2);
		if (err < 0)
			return (err);
	}
	int			prep(void)
	{
		// from conn.body
		// env
		// cmd_args
		return (0);
	}
	int			exec(void) // post-fork
	{
		int	err;
		
		pid_t pid = fork();
		if (pid == -1)
			return (-1);
		if (pid == 0)
		{
			close(p1[1]);
			err = dup2(p1[0], STDIN_FILENO);
			if (err)
				return (err);
			close(p1[0]);
			
			close(p2[0]);
			err = dup2(p2[1], STDOUT_FILENO);
			if (err)
				return (err);
			close(p2[1]);

			// if (this->conn.serv.get_port() == 8080)
			// {
			// 	char	bin[] = "/usr/bin/python";
			// 	char	fil[] = "test.py";
			// 	char * args[] = 
			// 	{
			// 		bin,
			// 		fil,
			// 		NULL
			// 	};
			// 	execve(bin, args, NULL);
			// }
			// else
			// {
			// 	char	bin[] = "/usr/bin/perl";
			// 	char	fil[] = "test.pl";
			// 	char * args[] = 
			// 	{
			// 		bin,
			// 		fil,
			// 		NULL
			// 	};
			// 	execve(bin, args, NULL);				
			// }			
		}
		

		
	}
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

