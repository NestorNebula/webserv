/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:34 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/18 16:45:27 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_PIPE_HPP
# define CGI_PIPE_HPP

# include <unistd.h>
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

	void		conn_closed(void) { this->conn = NULL; }

private:
	Connection		*conn;
	// Session		*sess;
};



#endif