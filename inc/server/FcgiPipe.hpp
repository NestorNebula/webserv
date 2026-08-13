/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiPipe.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:14 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 14:54:06 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FCGI_PIPE_HPP
# define FCGI_PIPE_HPP

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <sys/un.h>

# include <iostream>
# include <string>

# include "MsgBuf.hpp"
# include "FcgiMsg.hpp"

# include "CgiEnv.hpp"

# include "WsLog.hpp"
# include "Socket.hpp"
# include "EpollClient.hpp"

# include "FcgiConn.hpp"

class Connection;
class ResourceFcgi;

class FcgiPipe : public EpollClient
{
private:
	FcgiPipe				(const FcgiPipe & that) : EpollClient(that), 
		conn(that.conn) {};
	FcgiPipe & operator=	(const CgiPipe & ) { return (*this); }
public:
	FcgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourceFcgi * _rsrc);
	~FcgiPipe();
	
	int			init(CgiEnv *cgienv);
	
	ssize_t		pollin (void);
	ssize_t		pollout(void);
	int			rdhup  (void);
	int			hup    (void);
	bool		timeo  (time_t);

	void		rsrc_closed(void);

private:
	FcgiConn		fcgi;
	Connection		*conn;
	ResourceFcgi	*rsrc;
};







#endif