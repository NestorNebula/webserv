/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:14 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 11:27:03 by kdonlon          ###   ########.fr       */
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
	static int		make_sock(const char * sock_path);
	
	FcgiMsgData		data; 
	std::string		req;
	std::string		rsp;

	FcgiConn() {}
	~FcgiConn() {}

	int				req_init(CgiEnv *env);
	void			req_body(char *buf, int siz);
	
	int				rsp_recv(char * buf, int siz);

private:
	static int		uid;
	int				rsp_data(char * buf, int cnt);

};

#endif