/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:14 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 10:20:08 by kdonlon          ###   ########.fr       */
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

class FcgiConn
{
private:
	FcgiConn			  (const FcgiConn & );
	FcgiConn & operator = (const FcgiConn & ) { return (*this); }

public:
	FcgiConn() {}
	~FcgiConn() {}
	
	static int		make_sock(const char * sock_path);
	
	FcgiMsgData		data; 
	std::string		req;
	std::string		rsp;


	int				req_init(CgiEnv *env);
	void			req_body(const char *buf, int siz);
	void			req_body(std::string & buf);
	int				rsp_recv(char *buf, int siz);

private:
	static int		uid;
	int				rsp_data(char *buf, int cnt);

};

#endif