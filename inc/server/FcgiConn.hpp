/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:14 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/11 12:40:15 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FCGI_CONN_HPP
# define FCGI_CONN_HPP

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

# ifndef BUILD_MAIN_FCGI
#  define BUILD_MAIN_FCGI 0
# endif 

# ifndef FCGI_DEBUG
#  define FCGI_DEBUG 1
# endif

#if 0
void find_pad(unsigned short len)
{
	unsigned short up8 = (len + 7) & ~(7);
	fprintf(stderr, "%i + (%i / %i) = %i\n", len, (up8 - len), 8 - (len & 0x7), up8);
}
#endif


class FcgiConn
{
public:
	FcgiMsgData	data; 

	FcgiConn() {}
	~FcgiConn() {}

	int		request(CgiEnv *env);
	void	push_body(char *buf, int siz);
	int		parse(char * buf, int siz);

	std::string	req;
	std::string rsp;

	static int	make_sock(const char * sock_path);
private:
	static int uid;
	int		push_data(char * buf, int cnt);

};

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
	
	ssize_t		pollin (void);
	ssize_t		pollout(void);
	int			rdhup  (void);
	int			hup    (void);
	bool		timeo  (time_t);

	void		rsrc_closed(void);

	int			init(CgiEnv *cgienv);
private:
	FcgiConn		fcgi;
	Connection		*conn;
	ResourceFcgi	*rsrc;
	int				have_body;
};







#endif