/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiConn.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/13 11:28:28 by kdonlon          ###   ########.fr       */
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

int FcgiConn::req_init(CgiEnv * env)
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

void FcgiConn::req_body(char *buf, int siz)
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

int FcgiConn::rsp_data(char * buf, int cnt)
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


int FcgiConn::rsp_recv(char * buf, int siz)
{
	char * chk = buf;
	char * end = buf + siz;

	WsLog::_(LVL_DBG, TGT_FCGI, "recv: ", siz);
	if (data.len)
	{
		if (data.len > siz)
		{
    		rsp_data(chk, siz);
    		return (1);
		}
		WsLog::_(LVL_DBG, TGT_FCGI, "parse: done");
		chk += rsp_data(chk, data.len);
		chk += data.pad; // not 100% CERTAIN we have enough for this
	}

	while (data.len == 0 && chk < end)
	{
		// if ((end - chk) < 8)
			// we're fucked

		WsLog::_(LVL_DBG, TGT_FCGI, "parse: rest ", end - chk);

		FcgiMsg * hed = (FcgiMsg*) chk;

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
    		chk += rsp_data(chk, data.len);
    		chk += hed->head.paddingLength;
    		continue;
		}
		WsLog::_(LVL_DBG, TGT_FCGI, "parse: partial");
		chk += 8; // ATTN : 
		chk += rsp_data(chk, (end - chk)); // negative (?)
    }
	WsLog::_(LVL_DBG, TGT_FCGI, "parse: complete");
    return 0;
}
