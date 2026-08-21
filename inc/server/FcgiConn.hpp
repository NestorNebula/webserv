/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:14 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/20 22:59:49 by kdonlon          ###   ########.fr       */
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
public:
	static int		make_sock(const char * sock_path);
	
	FcgiMsgData		data; 
	std::string		req;
	std::string		rsp;

	FcgiConn() {}
	~FcgiConn() {}

	int				req_init(CgiEnv *env);
	void			req_body(char *buf, int siz);
	int				rsp_recv(char *buf, int siz);

private:
	static int		uid;
	int				rsp_data(char *buf, int cnt);

};

#endif