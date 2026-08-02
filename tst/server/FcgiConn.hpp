#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/un.h>



#include <iostream>
#include <string>

#include "MsgBuf.hpp"
#include "FcgiMsg.hpp"

#include "WsLog.hpp"

#ifndef BUILD_MAIN_FCGI
#define BUILD_MAIN_FCGI 1
#endif 

#ifndef FCGI_DEBUG
#define FCGI_DEBUG 1
#endif

#if 0
void find_pad(unsigned short len)
{
	unsigned short up8 = (len + 7) & ~(7);
	fprintf(stderr, "%i + (%i / %i) = %i\n", len, (up8 - len), 8 - (len & 0x7), up8);
}
#endif

class FcgiRequest
{
public:
	char * path;
	char * meth;
	char * type;
	char * _uri;
	char * query;
	char * post;
	char * serv;
	char * cook;
	char * host;
	char * root;
	char * radr;
	int    port;
	int    plen;
	int    id;
	bool   ssl;

	FcgiRequest()
	{
		path  = NULL;
		meth  = NULL;
		type  = NULL;
		_uri  = NULL;
		query = NULL;
		post  = NULL;
		serv  = NULL;
		cook  = NULL;
		host  = NULL;
		root  = NULL;
		radr  = NULL;
		port  = 0;
		plen  = 0;
		id    = FcgiRequest::uid++;
		ssl   = false;
	}
	~FcgiRequest()
	{
		if (serv)
			free(serv);
		if (radr)
			free(radr);
	}
private:
	static int uid;
};


class FcgiConn
{
public:
	FcgiMsgData	data; 

	FcgiConn() {}
	~FcgiConn() {}

	int		request(FcgiRequest * req);
	void	push_body(char *buf, int siz);
	int		parse(char * buf, int siz);

	std::string	req_head;
	std::string	req_body;

	std::string ostr;

	static int	make_sock(const char * sock_path);
private:
	int		push_data(char * buf, int cnt);

};


