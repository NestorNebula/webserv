/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/11 12:39:48 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "FcgiConn.hpp"

#include "ResourceCgi.hpp"

#include <string>


int FcgiConn::uid = 1;

int FcgiConn::make_sock(const char *sock_path)
{
	struct sockaddr_un fpm;
	fpm.sun_family = AF_UNIX;
	strcpy(fpm.sun_path, sock_path);

	int fd = socket(fpm.sun_family, SOCK_STREAM, 0);
	if (fd < 0)
		return (WsLog::_errno(LVL_ERR, TGT_FCGI, "socket"));
	
    int err = connect(fd, (struct sockaddr*) &fpm, sizeof(struct sockaddr_un));
    if (err < 0)
	{
		close(fd);
		// return (WsLog::_errno(LVL_DBG, TGT_FCGI, "connect"));
		WsLog::_(LVL_DBG, TGT_FCGI, "connect");
		return (err);
	}
    return (fd);
}

#if 0

  'USER' => 'www-data',
  'HOME' => '/var/www',
  'FCGI_ROLE' => 'RESPONDER',
  'QUERY_STRING' => 'v=1',
  'REQUEST_METHOD' => 'GET',
  'CONTENT_TYPE' => '',
  'CONTENT_LENGTH' => '',
  'SCRIPT_FILENAME' => '/var/www/test.php',
  'SCRIPT_NAME' => '/test.php',
  'PATH_INFO' => '/foo/bar.php',
  'REQUEST_URI' => '/test.php/foo/bar.php?v=1',
  'DOCUMENT_URI' => '/test.php/foo/bar.php',
  'DOCUMENT_ROOT' => '/var/www',

  'SERVER_PROTOCOL' => 'HTTP/1.1',
  	// does this need to be set for (HTTP/1.1 200 OK) to be sent ?
	// no .. the problem is that php-fpm generates status headers in the form
	// Status: RESP_CODE Response Message

  'GATEWAY_INTERFACE' => 'CGI/1.1',
  'SERVER_SOFTWARE' => 'nginx/1.4.0',
  'REMOTE_ADDR' => '192.168.56.1',
  'REMOTE_PORT' => '44644',
  'SERVER_ADDR' => '192.168.56.3',
  'SERVER_PORT' => '80',
  'SERVER_NAME' => '',
  'HTTPS' => '',
  'REDIRECT_STATUS' => '200',
  'HTTP_HOST' => 'lemp.test',
  'HTTP_USER_AGENT' => 'Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:20.0) Gecko/20100101 Firefox/20.0',
  	// maybe
  'HTTP_ACCEPT' => 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
  'HTTP_ACCEPT_LANGUAGE' => 'en-US,en;q=0.5',
  'HTTP_ACCEPT_ENCODING' => 'gzip, deflate',
  'HTTP_CONNECTION' => 'keep-alive',
  'PHP_SELF' => '/test.php/foo/bar.php',
  'REQUEST_TIME' => 1367829847,


    fastcgi_param CONTENT_TYPE $content_type;
    fastcgi_param CONTENT_LENGTH $content_length;



COMMON VARIABLES



    $query_string or $args: The arguments given in the original client request.

    $is_args: Will equal (?) if there are arguments in the request and will be set to an empty string otherwise. This is useful when constructing parameters that may or may not have arguments.

    $request_method: This indicates the original client request method. This can be useful in determining whether an operation should be permitted within the current context.

    $content_type: This is set to the Content-Type request header. This information is needed by the proxy if the users request is a POST in order to correctly handle the content that follows.

    $content_length: This is set to the value of the Content-Length header from the client. This information is required for any client POST requests.

    $fastcgi_script_name: This will contain the script file to be run. If the request ends in a slash (/), the value of the fastcgi_index directive will be appended to the end. In the event that the fastcgi_split_path_info directive is used, this variable will be set to the first captured group defined by that directive. The value of this variable should indicate the actual script to be run.

    $request_filename: This variable will contain the file path for the requested file. It gets this value by taking the value of the current document root, taking into account both the root and alias directives, and the value of $fastcgi_script_name. This is a very flexible way of assigning the SCRIPT_FILENAME parameter.

    $request_uri: The entire request as received from the client. This includes the script, any additional path info, plus any query strings.

    $fastcgi_path_info: This variable contains additional path info that may be available after the script name in the request. This value sometimes contains another location that the script to execute should know about. This variable gets its value from the second captured regex group when using the fastcgi_split_path_info directive.

    $document_root: This variable contains the current document root value. This will be set according to the root or alias directives.

    $uri: This variable contains the current URI with normalization applied. Since certain directives that rewrite or internally redirect can have an impact on the URI, this variable will express those changes.

#endif

int FcgiConn::request(CgiEnv * env)
{
	data.zero();

	FcgiMsg		msg;

	msg.new_params(FcgiConn::uid++);
	char proto[] = "HTTP/1.1";
	msg.add_param("SERVER_PROTOCOL", proto);

	// validate

	// REQUEST_URI : how did HttpServer build it 
	// DOCUMENT_ROOT

	std::map<std::string, std::string>::iterator kvit = env->kv.begin();
	while (kvit != env->kv.end())
	{

		// char * tok = strtok(req->cook, "; ");
		// while( tok != NULL )
		// {
			// WsLog::_(LVL_DBG, TGT_FCGI, "cookie: ", tok);
		// 	msg.add_param("HTTP_COOKIE", tok);
		// 	tok = strtok(NULL, "; ");
		// }
		// msg.add_param("HTTP_COOKIE", req->cook);

		// WsLog::_(LVL_DBG, TGT_FCGI, "pkey:  ", (kvit->first).c_str());
		// WsLog::_(LVL_DBG, TGT_FCGI, "pval:  ", (kvit->second).c_str());
		msg.add_param((const char*) (kvit->first).c_str(), (char*) (kvit->second).c_str());
		kvit++;
	}
	msg.end_params();
	
	req.append(msg.buf.text(), msg.buf.size());

	return (0);
}

void FcgiConn::push_body(char *buf, int siz)
{
	FcgiMsg		body;
	
	if (buf)
		body.add_stdin(buf, siz);
	else
		body.end_stdin();
	
	req.append(body.buf.text(), body.buf.size());
}


// A Responder performing an update, e.g. implementing a POST method, should compare the number of bytes received on FCGI_STDIN with CONTENT_LENGTH and abort the update if the two numbers are not equal.

/*
The start line of an HTTP response, called the status line, contains the following information:

    The protocol version, usually HTTP/1.1.

    A status code, indicating success or failure of the request. Common status codes are 200, 404, or 302

    A status text. A brief, purely informational, textual description of the status code to help a human understand the HTTP message.
*/

int FcgiConn::push_data(char * buf, int cnt)
{
	switch(data.typ)
	{
	case FCGI_STDERR:
		WsLog::color(WSL_YELLOW);
		WsLog::_(LVL_DBG, TGT_FCGI, "push data : error");
		WsLog::_(LVL_DBG, TGT_FCGI, "**** ****\n", buf);
		break;
	case FCGI_END_REQUEST:
		WsLog::_(LVL_DBG, TGT_FCGI, "push data : end cnt ", cnt);
		WsLog::_(LVL_DBG, TGT_FCGI, "push data : end len ", data.len);
		// should have (8) bytes of FCGI_EndRequestBody
		break;
	case FCGI_STDOUT:
		rsp.append(buf, cnt);
		break;
	default:
		WsLog::_(LVL_DBG, TGT_FCGI, "push data : default ", data.typ);
		break;
	}
	data.len -= cnt;
	return cnt;
}


int FcgiConn::parse(char * buf, int siz)
{
	char * chk = buf;
	char * end = buf + siz;

	WsLog::_(LVL_DBG, TGT_FCGI, "parse: ", siz);
	if (data.len)
	{
		if (data.len > siz)
		{
    		push_data(chk, siz);
    		return (1);
		}
		WsLog::_(LVL_DBG, TGT_FCGI, "parse: done");
		chk += push_data(chk, data.len);
		chk += data.pad; // not 100% CERTAIN we have enough for this
	}

	while (data.len == 0 && chk < end)
	{
		// if ((end - chk) < 8)
			// we're fucked

		WsLog::_(LVL_DBG, TGT_FCGI, "parse: rest ", end - chk);

		FcgiMsg * hed = (FcgiMsg*) chk;

		// hed->info();
		hed->data(&data);

		if (hed->head.type == FCGI_END_REQUEST)
		{
			WsLog::_(LVL_DBG, TGT_FCGI, "parse: end ", end - chk);
			WsLog::_(LVL_DBG, TGT_FCGI, "parse: len ", data.len);
			return (2);
		}

		WsLog::_(LVL_DBG, TGT_FCGI, "parse: need ", data.siz);
		WsLog::_(LVL_DBG, TGT_FCGI, "parse: have ", end - chk);

		if (data.siz <= (end - chk))
		{
			WsLog::_(LVL_DBG, TGT_FCGI, "parse: full");
			chk += 8;
    		chk += push_data(chk, data.len);
    		chk += hed->head.paddingLength;
    		continue;
		}
		WsLog::_(LVL_DBG, TGT_FCGI, "parse: partial");
		chk += 8; // ATTN : 
		chk += push_data(chk, (end - chk)); // negative (?)
    }
	WsLog::_(LVL_DBG, TGT_FCGI, "parse: complete");
    return 0;
}





#if BUILD_MAIN_FCGI

int main(void)
{

	char sock_path[] = "./FCGI/.php-fpm/SOCK"; // DANGEROUS
	// char sock_path[] = "/run/php/php-fpm.sock";
	int fd = FcgiConn::make_sock(sock_path);
	if (fd < 0)
		return (1);

	WsLog::lvl = LVL_ALL;
	WsLog::tgt = TGT_FCGI;

	FcgiConn fcgi; // sock_path

//      fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
//      fastcgi_param SERVER_NAME $http_host;
// +    fastcgi_param SCRIPT_NAME $request_uri;
// fastcgi_param  PATH_INFO  $request_uri;
        // fastcgi_param  SCRIPT_FILENAME /usr/share/nginx/www/$fastcgi_script_name;
        // fastcgi_param  PATH_INFO       $fastcgi_path_info;
        // fastcgi_param  PATH_TRANSLATED $document_root$fastcgi_script_name;
        // fastcgi_pass   unix:/var/run/php/php7.0-fpm.sock;
	// char p[] = "/home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php";

	// char p[] = "/media/kdonlon/data/Documents/42/webserv/git/tst/server/test.php";
	// char p[] = "./test.php";z

	char p[] = "/home/kdonlon/Documents/Projects/webserv/git/tst/server/test.php";

	char meth[] = "POST";
	char ptype[] = "application/x-www-form-urlencoded";
	char pdata[] = "p1=FCGI-post-one&p2=FCGI-post-two";
	int  plen = strlen(pdata);

	char q[] = "g1=fcgi-ONE&g2=fcgi-TWO";
	

	CgiEnv e;
	e.add("REQUEST_METHOD", meth);
	// e.add("DODCUMENT_ROOT", root);
	e.add("SCRIPT_FILENAME", p);
	e.add("QUERY_STRING", q);
	e.add("CONTENT_TYPE", ptype);
	e.add("CONTENT_LENGTH", plen);
	
// POLLIN : head
	int err = fcgi.request(&e); 	
	if (err < 0)
		return (1);

#define BUF_SIZ (1024)

	int bs;
	
	bs = send(fd, fcgi.req.c_str(), fcgi.req.size(), 0);
	WsLog::_(LVL_DBG, TGT_FCGI, "sent ", bs);
	WsLog::_(LVL_DBG, TGT_FCGI, " of  ", (int) fcgi.req.size());

// POLLIN : body
	fcgi.push_body(pdata, plen);
	fcgi.push_body(NULL, 0);

// perhaps : store as std::string in (msg)
	// std::string req_body;
	// req_body.append(fcgi.body.buf.text(), fcgi.body.buf.size());

	bs = send(fd, fcgi.req.c_str(), fcgi.req.size(), 0);
	WsLog::_(LVL_DBG, TGT_FCGI, "sent ", bs);
	WsLog::_(LVL_DBG, TGT_FCGI, " of  ", (int) fcgi.req.size());



// POLLOUT
	char buf[BUF_SIZ];
	// recv in CHUNKS
	while (1)
	{
		int siz = recv(fd, buf, BUF_SIZ, 0);
		if (siz <= 0)
			break;
		if (fcgi.parse(buf, siz) < 0)
			break;
		// conn->cgi_data() .. post-parsed (?)
		
	}
	WsLog::_(LVL_DBG, TGT_FCGI, "ostr\n", fcgi.ostr);

	close(fd);
	return (0);
}
#endif













FcgiPipe::FcgiPipe (Epoll *_ep, int _fd, Connection * _conn, ResourceFcgi * _rsrc) : 
	EpollClient(_ep, EPC_FCGI, _fd), 
	conn(_conn),
	rsrc(_rsrc),
	have_body(0)
{
	sock_non_block(this->fd);
}
	
FcgiPipe::~FcgiPipe()
{
	WsLog::_(LVL_DBG, TGT_FCGI, " (~) Fcgi");
	if (this->conn)
		this->conn->cgi_rem(this);
	// if (this->rsrc)
	// 	this->rsrc->rem(this);
}

bool	FcgiPipe::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CGI_TIMEOUT) < now)
	{
		if (this->rsrc)
			this->rsrc->set_err(504);
		else if (this->conn)
			this->conn->set_err(504);
		return (true);
	}
	return (false);
}

int		FcgiPipe::init(CgiEnv * cgienv)
{
	int err;

	err = fcgi.request(cgienv);
	if (err < 0)
		return (err);
	return (err);
}

ssize_t	FcgiPipe::pollin(void)
{
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);

	ssize_t	err = 0;
	
	WsLog::_(LVL_DBG, TGT_FCGI, "recv");
	err = this->recv();
	WsLog::_(LVL_DBG, TGT_FCGI, "recv: ", err);

	// hm : data returned from CGI .. BEFORE "upload" is complete ... 
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "recv: err");
		this->rsrc->set_err(501); // CGI_ERR : read failed
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_FCGI, "recv:  ZERO");
		WsLog::_(LVL_DBG, TGT_FCGI, "req : ", this->fcgi.req.size());
		WsLog::_(LVL_DBG, TGT_FCGI, "body: ", this->conn->req_body_status());

		// wtf : recev
		if ((this->fcgi.req.size() > 0) || (this->conn->req_body_status() >= 0))
		{
			// this->mod_evt(-EPOLLIN);
			return (0);
		}
		return (0);
	}
	
	if (fcgi.parse(this->ibuf, err) < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "fcgi:  parse failed");
		return (-1);
	}
		
	// why are we getting output from the CGI ... 
	// when we have not yet finished writing the upload data ... 
	
	switch (this->rsrc->recv_data((char*) fcgi.rsp.c_str(), fcgi.rsp.size()))
	{
	case RSRC_RESP_INIT:
		break;
	case RSRC_RESP_ERR:
		conn->set_err(rsrc->error);
		break;
	case RSRC_RESP_HEAD:
		// this->mod_evt(EPOLLOUT);
		break;
	case RSRC_RESP_BODY:
	default:
		// this->mod_evt(EPOLLOUT);
		conn->mod_evt(EPOLLOUT);
		break;
	}
	fcgi.rsp.clear();
	return (err);
}

// The server is in no way obligated to send end-of-file 
// after the script reads CONTENT_LENGTH bytes. 

// static int body_push = 0;

ssize_t	FcgiPipe::pollout(void)
{
	ssize_t	err;
	
	if (this->conn == NULL)
		return (-1);
	if (this->rsrc == NULL)
		return (-1);
	

	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT: ", fcgi.req.size());
	// WsLog::_(LVL_DBG, TGT_FCGI, "POUT\n", fcgi.req);
// WEBSERV : SESSION
	if (this->conn->req_body_status() > 0)
	{
		std::string & body = this->conn->sess.req.get_body();
		
		WsLog::_(LVL_DBG, TGT_FCGI, "body: ", body.size());

// body_push += body.size();
if (body.size() == 0)
{
	WsLog::color(WSL_RED);

	WsLog::_(LVL_DBG, TGT_FCGI, "body: ZERO");
}
		fcgi.push_body((char*) body.c_str(), body.size());
		body.clear(); 
	}
	if (!have_body && this->fcgi.req.size() == 0)
	{
		err = this->conn->req_body_status();
		if (err < 0)
		{
			WsLog::_(LVL_DBG, TGT_FCGI, "body     : complete");
			fcgi.push_body(NULL, 0);
			this->mod_evt(-EPOLLOUT);
			have_body = 1;
		}
		else if (err == 0)
		{
			// Continue -- should FAIL
			WsLog::_(LVL_DBG, TGT_FCGI, "body     : waiting");
			this->mod_evt(0);
			return (0);
		}
		else
		{
	// UPLOAD
	// conn read all data from client => req.body
	// conn returned data to   client
	// req.body .. has not been fully pushed to fcgi ...
	// 

// should we not be TRYING to read .. until all data is sent (?)

	// may : always want to get body .. 
// WEBSERV : SESSION
			std::string & body = this->conn->sess.req.get_body();
			
			WsLog::_(LVL_DBG, TGT_FCGI, "send: ", body.size());

	// body_push += body.size();
			fcgi.push_body((char*) body.c_str(), body.size());
			body.clear();
			// WsLog::_(LVL_DBG, TGT_FCGI, "fcgi: body\n", fcgi.req_body);
		}
	}
// epoll : evt tgt  : conn
// epoll : evt fd   : [7]
// epoll : evt typ  : in 
// conn  : recv
// epc   : read: [4096]
// conn  : recv: [4096]
// body  : blen: [32565]
// body  : clen: [463500]
// epoll : cli mod  : fcgi
// epoll : 
// epoll : evt tgt  : fcgi
// epoll : evt fd   : [8]
// epoll : evt typ  : out 
// epc   : send: [4104]
// epc   : sent: [4096]
// fcgi  : sent: [4096]
// epoll : cli mod  : fcgi
// epoll : 
// ecnt  : [2]
// epoll : 
// epoll : evt tgt  : conn
// epoll : evt fd   : [7]
// epoll : evt typ  : in 
// conn  : recv
// epc   : read: [4096]
// conn  : recv: [4096]
// body  : blen: [36661]
// body  : clen: [463500]
// epoll : cli mod  : fcgi
// epoll : 
// epoll : evt tgt  : fcgi
// epoll : evt fd   : [8]
// epoll : evt typ  : out 
	// strange .. 
// epc   : send: [8]
// epc   : sent: [8]
// fcgi  : sent: [8]
// epoll : cli mod  : fcgi
// epoll : 
// ecnt  : [2]

// WsLog::_(LVL_DBG, TGT_FCGI, "body:  pushed ", body_push);
	err = this->send(fcgi.req);
// -pass-header Authorization
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_FCGI, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_FCGI, "send:  ZERO");
		return (0);
	}
	WsLog::_(LVL_DBG, TGT_FCGI, "sent: ", err);
	WsLog::_(LVL_DBG, TGT_FCGI, "left: ", fcgi.req.size());

	// if (fcgi.req.size() == 0)
	this->mod_evt(EPOLLIN);
	// if (have_body)
	// 	return (-1); // did we need to close here (?)

// are we done ?

	return (0);
}

int		FcgiPipe::rdhup(void)
{
	// nothing more to "send back"
	// but .. still may be receiving an upload
	// this->mod_evt(-EPOLLIN); // BAD IDEA
	// this->mod_evt(-EPOLLOUT);
	WsLog::_(LVL_DBG, TGT_FCGI, "RDHUP");
	if (this->fcgi.req.size())
		return (0);
	if (have_body == 0)
		return (0);
	// still need to send BODY_DONE 
	if (this->rsrc->ostr.size())
		return (0);
	if (this->conn->req_body_status() >= 0)
		return (0);
		
	return (-1);
}

int		FcgiPipe::hup(void)
{
	return (-1);
}

void	FcgiPipe::rsrc_closed(void)
{ 
	this->conn = NULL;
	this->rsrc = NULL;
}



