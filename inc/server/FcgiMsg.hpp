/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiMsg.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 19:36:11 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 10:21:17 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FCGI_MSG_HPP
# define FCGI_MSG_HPP

# include <unistd.h>
# include <stdio.h>
# include <string.h>
# include <iostream>
# include <fastcgi.h>

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






// typedef struct {
//     unsigned char version;
//     unsigned char type;
//     unsigned char requestIdB1;
//     unsigned char requestIdB0;
//     unsigned char contentLengthB1;
//     unsigned char contentLengthB0;
//     unsigned char paddingLength;
//     unsigned char reserved;
// } FCGI_Header;


	// FCGI_BeginRequestBody
// typedef struct {
//     unsigned char roleB1;
//     unsigned char roleB0;
//     unsigned char flags;
//     unsigned char reserved[5];
// } FCGI_BeginRequestBody;

// typedef struct {
//     FCGI_Header header;
//     FCGI_BeginRequestBody body;
// } FCGI_BeginRequestRecord;


// typedef struct {
//     unsigned char appStatusB3;
//     unsigned char appStatusB2;
//     unsigned char appStatusB1;
//     unsigned char appStatusB0;
//     unsigned char protocolStatus;
//     unsigned char reserved[3];
// } FCGI_EndRequestBody;

// typedef struct {
//     FCGI_Header header;
//     FCGI_EndRequestBody body;
// } FCGI_EndRequestRecord;

#if 0
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)
#endif


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