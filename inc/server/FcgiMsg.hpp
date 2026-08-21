/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiMsg.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 19:36:11 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 17:15:09 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FCGI_MSG_HPP
# define FCGI_MSG_HPP

# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <iostream>

# include "FcgiDefs.hpp"
# include "MsgBuf.hpp"
# include "WsLog.hpp"

class FcgiMsgData
{
private:
	FcgiMsgData				 (const FcgiMsgData & );
	FcgiMsgData & operator = (const FcgiMsgData & ) { return (*this); }
public:
	int				siz;  // full_size
	int				len;  // msg_len

	int				req;  // req_id
	int				role; // not sure when

	unsigned char	typ;
	unsigned char	pad;

	FcgiMsgData();
	~FcgiMsgData() {};
	
	void zero();
};

	// can lose this
enum
{
	P_SERVER_PROTOCOL,
	P_REQUEST_METHOD,
	P_SCRIPT_FILENAME,
	P_CONTENT_TYPE,
	P_QUERY_STRING,
	P_CONTENT_LENGTH,
	P_HTTP_ACCEPT,
	P_MIME_TYPE,
	P_SERVER_ADDR,
	P_REQUEST_URI,
	P_SERVER_PORT,
	P_HTTP_COOKIE,
	P_HTTP_HOST,
	P_HTTPS,
	P_DOCUMENT_ROOT,
	P_REMOTE_ADDR
};

class FcgiMsg
{
private:
	FcgiMsg				 (const FcgiMsg & );
	FcgiMsg & operator = (const FcgiMsg & ) { return (*this); }
public:
	FCGI_Header           head;
	FCGI_BeginRequestBody body;

	MsgBuf        buf;
	unsigned int pHed; // offset of FCGI_PARAMS header
	unsigned int pBeg; // offset of inserted params (k/v)

	FcgiMsg();
    ~FcgiMsg() {}

	void 	new_params(unsigned short req = 0x1);

	void 	add_param(const char * key, const char * val);
	void 	add_param(const char * key, int val);
	void 	end_params(void);
	
	void	add_stdin(const char *buf, int siz);
	void	end_stdin(void);

	void 	data(FcgiMsgData * data);
	
private:
	void 	begin();
	void 	make_head(unsigned char typ, unsigned short len);

	int  	full_size();

	void           set_pad(unsigned short len);

	void           set_req(unsigned short req);
	unsigned short get_req();

	void           set_len(unsigned short len);
	unsigned short get_len();

	void           set_role(unsigned short role);
	unsigned short get_role();

	void 	zero();
	void 	info();
};



#endif