/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:34 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/21 15:24:47 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_PIPE_HPP
# define CGI_PIPE_HPP

# include <unistd.h>
# include <cerrno>
# include <cstdio>
# include <cstring>
# include <iostream>

# include "EpollClient.hpp"

class cgi_pipes
{
private:
	cgi_pipes				(const cgi_pipes & ) {} 
	cgi_pipes & operator =	(const cgi_pipes & ) { return (*this); }
public:

	cgi_pipes (void);
	~cgi_pipes();
	
	int		init(void);
	int		dup_io(void);
	int		dup_err(void);
	void	shutdown(void);

	int		p1[2];
	int		p2[2];
	int		dnfd;
};

class Connection;

class CgiPipe : public EpollClient
{
private:
	CgiPipe				(const CgiPipe & that) : EpollClient(that), 
		conn(that.conn) {};
	CgiPipe & operator=	(const CgiPipe & ) { return (*this); }
public:
	CgiPipe (Epoll *_ep, int _fd, Connection * _conn);
	~CgiPipe();
	
	ssize_t		pollin (void);
	ssize_t		pollout(void);
	int			hup    (void);
	bool		timeo  (time_t);

	void		conn_closed(void);

private:
	Connection		*conn;
	// Session		*sess;
};

class ResourceCgi
{
private:
	ResourceCgi				 (const ResourceCgi & ) {}
	ResourceCgi & operator = (const ResourceCgi & ) { return (*this); }
public:
	ResourceCgi(void) : pid(0), ip(NULL), op(NULL), stat(-1), hed(0), xit(-1), sig(-1) {}
	~ResourceCgi();
	
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
	int			stat;
	int			hed;
	int			xit;
	int			sig;

	int			status(int opt);
	int			rem(CgiPipe *epc);
	void		reset(void);
};


#endif