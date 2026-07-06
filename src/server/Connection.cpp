/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 21:20:54 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (Epoll & _ep, int _fd, Server &_serv) : 
	EpollClient(_ep, EPC_CONN, _fd), 
	serv(_serv), 
	req_cnt(0),
	state(0)
{
};

Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
};

// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 

// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 

// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));		
}

std::string Connection::header(const char *key)
{
	std::string val("");
	
	std::string kstr(key);
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
		// maybe not (-1) : just turn off pollin
		return (0);
	}

	istr.append(this->ibuf, err); // std::string::append includes any (null)
	WsLog::_(LVL_DBG, TGT_CONN_RECV, "istr: ", istr.size());
	
	if (this->state < CONN_HAS_HEAD)
	{
		std::string hed_end("\r\n\r\n");
		
		size_t	crlf = istr.find(hed_end);
		if (crlf == std::string::npos)
			return (err);
		
		head = istr.substr(0, crlf + 4);
		istr.erase(0, crlf + 4);
		
		WsLog::_(LVL_INFO, TGT_CONN_DATA, "head");
		WsLog::_(LVL_INFO, TGT_CONN_DATA, "****\n", head);
		this->state = CONN_HAS_HEAD;
		this->req_cnt++;
	}
	
	if (this->state < CONN_HAS_RSRC)
	{
		this->resp = std::string("HTTP/1.1 200 OK\r\n");
// KEEP_ALIVE
		// this->resp = std::string("HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\n");
		err = this->exec_cgi(); // (this->head)
		if (err < 0)
		{
			WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
			return (err);
		}
		this->state = CONN_HAS_RSRC;
	}
	// rsrc.push(ibuf, err)
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
		return (0);
	}
	if (this->state < CONN_SENT_RESP)
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
		if (this->state == RSRC_SENT_BODY) // complete
		{
#if 0 // KEEP_ALIVE -- would REQUIRE CONTENT-LENGTH from (cgi)
			// see more HUP => read [0] with THIS .. AND NOT siege.conf
			this->istr.clear();
			this->state = 0;
			this->mod_evt(EPOLLIN); // something more here ..
			return (0);
#else
			// TEST : content-length header .. longer than what we send
			// curl: (18) end of response with 5 bytes missing
			return (-1);		
#endif			
		}
		this->mod_evt(0); // otherwise, we get stuck here 
		return (0);
	}
	if (this->state < RSRC_SENT_BODY)
	{
		this->mod_evt(0);
		return (0);
	}
	
// KEEP_ALIVE
	// if (this->state < CONN_SENT_LENGTH)
	// {
			// wrong : if php sent headers .. ugh
	// 	WsLog::_(LVL_DBG, TGT_CONN_SEND, "clen: ", ostr.size());
	// 	std::string len("Content-Length: " + num_2_str(ostr.size()) + "\r\n");
	// 	err = this->send(len);
	// 	this->state = CONN_SENT_LENGTH;
	// 	return (0);
	// }
	// // we sent LENGTH
	// wait until cgi says done .. 

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






CgiEnv::CgiEnv(void) : res(NULL)
{

}

CgiEnv::~CgiEnv()
{
	if (res)
		delete[] res;
}

// from_headers
void	CgiEnv::add(const char *key, const char *val)
{
	// <map> first .. to override multiple (?)
	data.push_back(std::string(key) + std::string("=") + std::string(val));
}

void	CgiEnv::add(const char *key, int n)
{
	data.push_back(std::string(key) + std::string("=") + num_2_str(n));
}

const char	**CgiEnv::gen(void)
{
	if (res)
		delete[] res;
	size_t	cnt	= data.size();

	res = new const char*[cnt + 1];
	const char	**ins = res;
	
	std::vector<std::string>::iterator it = data.begin();
	while (it != data.end())
	{
		*ins++ = it->c_str();
		it++;
	}
	*ins = NULL;
	return (res);
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

int	Connection::exec_cgi(void)
{
	int		err;

	int		p1[2];
	int		p2[2];

	err = pipe(p1);
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipe");
	err = pipe(p2);
	if (err < 0)
	{
		// cleanup
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipe");
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		// cleanup (!)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}

	if (pid == 0)
	{
		// Epoll:: CLOEXEC should have closed Epoll::epfd
		close(p1[1]);
		err = dup2(p1[0], STDIN_FILENO);
		if (err < 0)
		{
			// cleanup (!)
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p1[0]);
		
		close(p2[0]);
		err = dup2(p2[1], STDOUT_FILENO);
		if (err < 0)
		{
			// cleanup (!)
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p2[1]);


			// FROM HEADER / cgi_env
		std::string	path;
		std::string	file;
		
		// https://httpd.apache.org/docs/trunk/howto/cgi.html
//     For current Python versions, use the urllib.parse module to parse query strings and form data. For more complex applications, consider a lightweight WSGI framework, though that moves beyond the scope of traditional CGI.

		// #!/usr/bin/env python3
		// TEST CGI at server startup
		if (false) // this->serv.get_port() == 8080)
		{
			// path = std::string("/usr/bin/python");
			// file = std::string("test.py");
			path = std::string("/usr/bin/perl");
			file = std::string("test.pl");
		}
		else
		{
			path = std::string("/usr/bin/php-cgi");
			file = std::string("test.php");
		}

		char const *args[3];
		args[0] = path.c_str();
		args[1] = file.c_str();
		args[2] = NULL;

		// From the meta-variables thus generated, a URI, the 'Script-URI', can
//    be constructed.  This MUST have the property that if the client had
//    accessed this URI instead, then the script would have been executed
//    with the same values for the SCRIPT_NAME, PATH_INFO and QUERY_STRING
//    meta-variables.

// script-URI = <scheme> "://" <server-name> ":" <server-port>
//                    <script-path> <extra-path> "?" <query-string>

// terminate long-running script if client closes connection

		CgiEnv cgienv;

		cgienv.add("REDIRECT_STATUS", "1"); // [404] NOT FOUND .. hm .. no input file 
		

	//  PHP CGI depends on non-standard SCRIPT_FILENAME

// This PHP CGI binary was compiled with force-cgi-redirect enabled.  This
// means that a page will only be served up if the REDIRECT_STATUS CGI variable is
// set
		// seems like CGI NEEDS THIS -- to know to wait for BODY
		cgienv.add("REQUEST_METHOD", "POST"); 
// If the output of a form is being processed, check that CONTENT_TYPE
//    is "application/x-www-form-urlencoded" [18] or "multipart/form-data"
//    [16].  If CONTENT_TYPE is blank, the script can reject the request
//    with a 415 'Unsupported Media Type' error, where supported by the
//    protocol.

		std::string val;

		// cgienv.add("CONTENT_TYPE", "application/x-www-form-urlencoded"); // IMPORTANT
		// cgienv.add("CONTENT_TYPE", "multipart/form-data"); // ATTN : does (cgi) parse boundary (?)

		val = this->header("Content-type");
		if (val.size())
			cgienv.add("CONTENT_TYPE", val.c_str());
		val = this->header("Content-length");
		if (val.size())
			cgienv.add("CONTENT_LENGTH", val.c_str());
// should (conn) know about INCOMING contenth-length ... 
// if we do not have content-length .. need to CLOSE the socket
// after all data has been written

		cgienv.add("PATH_INFO", "path info"); // added to PHP_SELF (?)
// PATH_TRANSLATED
// Maps the script's virtual path to the physical path used to call the script. This is done by taking any PATH_INFO component of the request URI and performing any virtual-to-physical translation appropriate.
		// SCRIPT_NAME
		// Returns the part of the URL from the protocol name up to the query string in the first line of the HTTP request.
		cgienv.add("SCRIPT_NAME", "test.php");
		cgienv.add("SCRIPT_FILENAME", "test.php");
// 		
		
		// need to EXTRACT from URL
		cgienv.add("QUERY_STRING", "g1=get-one&g2=get-two");
		
			// get this from Conn::addr
		cgienv.add("REMOTE_ADDR", "remote addr");
		cgienv.add("REMOTE_HOST", "remote host");
		cgienv.add("REMOTE_USER", "remote user");
		
		val = this->header("Host");
		if (val.size())
			cgienv.add("HTTP_HOST", val.c_str());
		val = this->header("User-Agent");
		if (val.size())
			cgienv.add("HTTP_USER_AGENT", val.c_str());
		
		val = this->header("Accept");
		if (val.size())
			cgienv.add("HTTP_ACCEPT", val.c_str());
		cgienv.add("HTTP_COOKIE", "cookies");
		

// In addition to these, the header lines recieved from the client, if any, are placed into the environment with the prefix HTTP_ followed by the header name. Any - characters in the header name are changed to _ characters. The server may exclude any headers which it has already processed, such as Authorization, Content-type, and Content-length. If necessary, the server may choose to exclude any or all of these headers if including them would exceed any system environment limits. 

	
			// CONSTANT : from server
		cgienv.add("SERVER_NAME", "webserv"); // virtual
		cgienv.add("SERVER_PORT", this->serv.get_port());
		cgienv.add("SERVER_PROTOCOL", "HTTP/1.1");
		cgienv.add("SERVER_SOFTWARE", "webserv");


		// need to ADD TO .. current (envp) ? 
		// cwd() !!!
		const char **envp = cgienv.gen();

		err = execve(args[0], (char* const*) args, (char* const*) envp);
		
		// cool : this (cout) gets READ by conn
		//  but : what do we really need to monitor in this case 

// WORK : on ibuf/obuf communication FIRST
		std::cout << "\n\nwtf\n\n";


// cgi HUP .. on python exception
		// bad executable -- how to handle this (?) actually TWICE (?)
		exit (err);
	}		
	
	WsLog::_(LVL_INFO, TGT_CONN, "exec cgi");

	close(p1[0]);
	close(p2[1]);


	// so .. conn->resource .. 
	// tracks these (?)
	int			cgifd;
	EpollClient	*epc_cgi;
	
	cgifd = dup(p1[1]);
	if (cgifd < 0)
	{
		// cleanup (!)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	}
	close (p1[1]);

	epc_cgi = new CgiPipe(this->ep, cgifd, *this);
	err = this->ep.add(epc_cgi, EPOLLOUT);
	if (err < 0)
	{
		// cleanup (!)
		return (err);
	}

	cgifd = dup(p2[0]);
	if (cgifd < 0)
	{
		// cleanup (!)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	}
	close (p2[0]);
	
	epc_cgi = new CgiPipe(this->ep, cgifd, *this);
	err = this->ep.add(epc_cgi, EPOLLIN);
	if (err < 0)
	{
		// cleanup (!)
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

