/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 20:31:57 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (Epoll *_ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	cgi_ip(NULL),
	cgi_op(NULL),
	serv(_serv), 
	req_cnt(0),
	state(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);

	// if either open .. 
	// should we try to KILL it (?)
	if (this->cgi_ip)
		this->cgi_ip->conn = NULL;
	if (this->cgi_op)
		this->cgi_op->conn = NULL;
};

// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 

// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 

// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));		
}

// more generic .. Request;
std::string Connection::header(const char *key)
{
	std::string val("");
	
	std::string kstr(key);

	if (kstr == std::string("METH"))
	{
		std::string			meth;
		std::stringstream	line(head);
		line >> meth;
		WsLog::_(LVL_DBG, TGT_HEAD, "meth: ", meth);
		return (meth);
	}
	if (kstr == std::string("PATH"))
	{
		std::string			meth;
		std::string			path;
		std::stringstream	line(head);
		line >> meth >> path;
		WsLog::_(LVL_DBG, TGT_HEAD, "path: ", path);
		return (path);
	}
	std::string::const_iterator it = std::search(
		head.begin(), head.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == head.end())
	{
		// std::cerr << "header: not found\n";
		return (val);
	}
	it += strlen(key);
	if (*it != ':')
	{
		// std::cerr << "header: not colon\n";
		return (val);
	}
	it++;
	it++; // space (ugly) BUT IMPORTANT
	size_t off_beg = (it - head.begin());

	std::string hed_eol("\r\n");
	size_t	off_end = head.find(hed_eol, off_beg);

	if (off_end == std::string::npos)
	{
		return (val);
	}

	val = head.substr(off_beg, off_end - off_beg);

	// ugh : need to point to something in head .. which is valid outside this function
	// std::cerr << "header:\nheader: " << key << "=" << val << std::endl;

	std::string kv(key + std::string("=") + val);
	WsLog::_(LVL_DBG, TGT_HEAD, kv);
	return (val);
}



#define CONN_TIMEO 5

// CgiPipe as well .. or .. just .. NOT SERVER
bool	Connection::timeo(time_t now)
{
	if (this->lact == 0)
		return (false);
	if (now < this->lact)
		return (false);
	if ((this->lact + CONN_TIMEO) < now)
		return (true);
	return (false);
}


ssize_t	Connection::pollin(void)
{
	ssize_t	err;

	WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: zero");
		return (0);
	}

	istr.append(this->ibuf, err);
	
	WsLog::_(LVL_DBG, TGT_CONN_RECV, "istr: ", istr.size());
	
	if (this->state < CONN_HAS_HEAD)
	{
		std::string hed_end("\r\n\r\n");
		
		size_t	crlf = istr.find(hed_end);
		if (crlf == std::string::npos)
			return (err);
		
		head = istr.substr(0, crlf + 4);
		istr.erase(0, crlf + 4);
		// parse chunked
		
		WsLog::_(LVL_DBG, TGT_HEAD, "head");
		WsLog::_(LVL_DBG, TGT_HEAD, "****\n", head);
		WsLog::_(LVL_DBG, TGT_HEAD, "rest");
		WsLog::_(LVL_DBG, TGT_HEAD, "****\n", istr);
		this->state = CONN_HAS_HEAD;
		this->req_cnt++;
	}
	
	if (this->state < CONN_HAS_RSRC)
	{
		switch(this->serv.get_port())
		{
		case 8080: // (php)
			this->resp = std::string("HTTP/1.1 200 OK\r\n");
			break;
		default:
			this->resp = std::string("HTTP/1.1 200 OK\r\n\r\n");
			break;
		}
		err = this->exec_cgi(); // (this->head)
		if (err < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			return (err);
		}
		this->state = CONN_HAS_RSRC;
	}
	return (err);
}


// ∗ Just remember that, for chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. If no content_length is returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

ssize_t	Connection::pollout(void)
{
	ssize_t	err = 0;

	if (this->state < CONN_HAS_RSRC)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no rsrc");
		return (0); // (-1)
	}
	// if (this->cgi_op == NULL)
		// ERROR
	// rsrc->state
	if (this->state < RSRC_HAS_RESP)
	{
		// get this more than I like ... 
		//back to mod_evt(-)
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: no resp");
		this->mod_evt(-EPOLLOUT); // why not (?)
		// should work with (-EPOLLOUT) .. right (?)
		// BUT : we want (0) to mean (0) ... 
		return (0); // (-1)
	}

	if (this->state < CONN_SENT_RESP || this->state == RSRC_BODY_DONE)
	{
		err = this->send(this->resp); 
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "resp: ", err);
		if (this->resp.size())
			return (err);
		this->state = CONN_SENT_RESP;
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");

	if (ostr.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");
		// rsrc.inProgress() .. wait for it
		// rsrc.failure() .. send error
		// rsrc.done()
		// not always getting this 
		if (this->state == RSRC_BODY_DONE || this->cgi_op == NULL) // complete
		{
#if 0 // KEEP_ALIVE -- would REQUIRE CONTENT-LENGTH from (cgi)
			// see more HUP => read [0] with THIS .. AND NOT siege.conf
			this->istr.clear();
			this->state = 0;
			// turn OFF first
			this->mod_evt(EPOLLIN);
			return (0);
#else
			// TEST : content-length header .. longer than what we send
			return (-1);		
#endif			
		}
		// lucky .. send has probably already happened 
		this->mod_evt(-EPOLLOUT); // otherwise, we get stuck here 
		return (0);
	}
	
	err = this->send(ostr);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN_SEND, "send");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: zero");
		return (0);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);	
	if (ostr.size())
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: all"); // may have more to fill 
	}

	return (err); // (!) bytes written
}







// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read.
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
	

// Resource .. built from Request
// CgiEnv   .. built from Request .. add to current ENV (?)

void	Connection::rem_cgi(CgiPipe *epc)
{
	// Q: error message (?)
	if (epc == this->cgi_ip) // expect this to close before (op)
		this->cgi_ip = NULL;
	else if (epc == this->cgi_op)
		this->cgi_op = NULL;
}
int	Connection::exec_cgi(void)
{
	int			err;
	cgi_pipes	pipes;

	if (pipes.init() < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipes");

	pid_t pid = fork();
	if (pid < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
		
	if (pid == 0)
	{
		err = pipes.dup_io();
		if (err < 0)
		{
			pipes.shutdown();
			delete (this->ep);
			exit(1);
		}

// FROM REQUEST
		std::string	path;
		std::string	file;
		
		switch(this->serv.get_port())
		{
		case 8081:
			path = std::string("/usr/bin/python");
			file = std::string("test.py");
			break;
		case 8082:
			path = std::string("/usr/bin/perl");
			file = std::string("test.pl");
			break;
		default:
			path = std::string("/usr/bin/php-cgi");
			file = std::string("test.php");
			break;
		}
		
		char const *args[3];
		args[0] = path.c_str();
		args[1] = file.c_str();
		args[2] = NULL;

// CHALLENGE
// terminate long-running script if client closes connection
		CgiEnv *cgienv = new CgiEnv;
		cgienv->from_conn(*this, file);

		const char **envp = cgienv->gen();

		// signal handler (!)
		// shit : ctrl-c .. leaves (php) RUNNING IN BACKGROUND 
		// and does not appear to cleanup connection to client 

// maybe, maybe not
// when client (conn) quits
// NEED THAT LINK between (conn) and (cgi)

// since it's still running .. that copy of server (port) is STILL OPEN 
		// signal(SIGINT, SIG_IGN); // (?)
		signal(SIGINT, SIG_DFL);
		// dangerous : should monitor their cleanup ..
		err = execve(args[0], (char* const*) args, (char* const*) envp);
		
		pipes.shutdown();
		delete (cgienv);
		delete (this->ep); 
		
// cgi HUP .. on python exception
		// bad executable -- how to handle this (?) actually TWICE (?)
		exit (err);
	}		
	

	
	WsLog::_(LVL_DBG, TGT_CONN, "exec cgi");

	int			cgifd_ip;
	int			cgifd_op;
	
	cgifd_ip = dup(pipes.p1[1]);
	if (cgifd_ip < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	cgifd_op = dup(pipes.p2[0]);
	if (cgifd_op < 0)
	{
		close(cgifd_ip);
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup (pipes)");
	}	

	this->cgi_ip = new CgiPipe(this->ep, cgifd_ip, this);
	err = this->cgi_ip->ini_evt(EPOLLOUT);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}

	this->cgi_op = new CgiPipe(this->ep, cgifd_op, this);
	err = this->cgi_op->ini_evt(EPOLLIN);
	if (err < 0)
	{
		close(cgifd_ip);
		close(cgifd_op);
		return (err);
	}
	return (err);
}






#if 0

AUTH_PASSWORD AUTH_TYPE AUTH_USER CERT_COOKIE CERT_FLAGS CERT_ISSUER CERT_KEYSIZE CERT_SECRETKEYSIZE CERT_SERIALNUMBER CERT_SERVER_ISSUER CERT_SERVER_SUBJECT CERT_SUBJECT CF_TEMPLATE_PATH CONTENT_LENGTH CONTENT_TYPE CONTEXT_PATH GATEWAY_INTERFACE HTTPS HTTPS_KEYSIZE HTTPS_SECRETKEYSIZE HTTPS_SERVER_ISSUER HTTPS_SERVER_SUBJECT HTTP_ACCEPT HTTP_ACCEPT_ENCODING HTTP_ACCEPT_LANGUAGE HTTP_CONNECTION HTTP_COOKIE HTTP_HOST HTTP_REFERER HTTP_USER_AGENT QUERY_STRING REMOTE_ADDR REMOTE_HOST REMOTE_USER REQUEST_METHOD SCRIPT_NAME SERVER_NAME SERVER_PORT SERVER_PORT_SECURE SERVER_PROTOCOL SERVER_SOFTWARE WEB_SERVER_API (This value is always blank; retained for compatibility.)

https://www6.uniovi.es/~antonio/ncsa_httpd/cgi/env.html


Specification

The following environment variables are not request-specific and are set for all requests:

    SERVER_SOFTWARE

    The name and version of the information server software answering the request (and running the gateway). Format: name/version

    SERVER_NAME

    The server`s hostname, DNS alias, or IP address as it would appear in self-referencing URLs.

    GATEWAY_INTERFACE

    The revision of the CGI specification to which this server complies. Format: CGI/revision

The following environment variables are specific to the request being fulfilled by the gateway program:

    SERVER_PROTOCOL

    The name and revision of the information protcol this request came in with. Format: protocol/revision

    SERVER_PORT

    The port number to which the request was sent.

    REQUEST_METHOD

    The method with which the request was made. For HTTP, this is "GET", "HEAD", "POST", etc.

    PATH_INFO

    The extra path information, as given by the client. In other words, scripts can be accessed by their virtual pathname, followed by extra information at the end of this path. The extra information is sent as PATH_INFO. This information should be decoded by the server if it comes from a URL before it is passed to the CGI script.

    PATH_TRANSLATED

    The server provides a translated version of PATH_INFO, which takes the path and does any virtual-to-physical mapping to it.

    SCRIPT_NAME

    A virtual path to the script being executed, used for self-referencing URLs.

    QUERY_STRING

    The information which follows the ? in the URL which referenced this script. This is the query information. It should not be decoded in any fashion. This variable should always be set when there is query information, regardless of command line decoding.

    REMOTE_HOST

    The hostname making the request. If the server does not have this information, it should set REMOTE_ADDR and leave this unset.

    REMOTE_ADDR

    The IP address of the remote host making the request.

    AUTH_TYPE

    If the server supports user authentication, and the script is protects, this is the protocol-specific authentication method used to validate the user.

    REMOTE_USER

    If the server supports user authentication, and the script is protected, this is the username they have authenticated as.

    REMOTE_IDENT

    If the HTTP server supports RFC 931 identification, then this variable will be set to the remote user name retrieved from the server. Usage of this variable should be limited to logging only.

    CONTENT_TYPE

    For queries which have attached information, such as HTTP POST and PUT, this is the content type of the data.

    CONTENT_LENGTH

    The length of the said content as given by the client.

In addition to these, the header lines recieved from the client, if any, are placed into the environment with the prefix HTTP_ followed by the header name. Any - characters in the header name are changed to _ characters. The server may exclude any headers which it has already processed, such as Authorization, Content-type, and Content-length. If necessary, the server may choose to exclude any or all of these headers if including them would exceed any system environment limits.

An example of this is the HTTP_ACCEPT variable which was defined in CGI/1.0. Another example is the header User-Agent.

    HTTP_ACCEPT

    The MIME types which the client will accept, as given by HTTP headers. Other protocols may need to get this information from elsewhere. Each item in this list should be separated by commas as per the HTTP spec.

    Format: type/subtype, type/subtype

    HTTP_USER_AGENT

    The browser the client is using to send the request. General format: software/version library/version.
	

https://www.ibm.com/docs/en/netcoolomnibus/8.1.0?topic=scripts-environment-variables-in-cgi-script



#endif

