/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 17:19:16 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (int _fd, Server &_serv) : EpollClient(EPC_CONN, _fd), serv(_serv), 
	req_cnt(0), filedes(-1)
{
};

Connection::Connection(const Connection & that) : EpollClient(EPC_CONN, that.fd), serv(that.serv),
	req_cnt(0), filedes(-1)
{
}


Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	// WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
	if (filedes != -1) // Resource
		close(filedes);
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
	std::string val;
	
	// std::cerr << "header: " << key << std::endl;
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
	std::cerr << "header:\nheader: " << key << "=" << val << std::endl;
	return (val);
}

int	Connection::pollin(void)
{
	int	err = 0;

	this->timeout();
	
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: zero");
		return (-1);
	}


// EpollBuf

// Yeah -- uploading file data ... 
// linked list of EpollBuf (?)
// need to track :: CUR .. beg/end .. flush .. next (?)

// gonna be hard to test BIG (binary) files without that approach in place 

	// copy (?) or keep-adding (?)
	istr += std::string(this->ibuf);
    ivec.insert(ivec.end(), ibuf, ibuf + err);
	
	// WsLog::_(LVL_INFO, TGT_CONN_RECV, "istr");
	// WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);
	

	std::string hed_end("\r\n\r\n");
	
	size_t	crlf = istr.find(hed_end);
	if (crlf == std::string::npos)
		return (err);

	this->req_cnt++;

	WsLog::_(LVL_INFO, TGT_CONN_RECV, "istr");
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);

	head = istr.substr(0, crlf + 4);
	// need to save head shit .. for passing to CGI
	istr.erase(0, crlf + 4); // ibuf.beg += .. 

	//  Reading data from STDIN is required when METHOD=POST, whereas when METHOD=GET, data are passed through the QUERY_STRING environment variable
		
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "body");
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);
	// for (size_t k=0; k < ivec.size(); k++)
	// 	std::cerr << ivec[k];
		
	
	// erase: up to \r\n\r\n
	// send rest as POST DATA



#define RESP_SIMPLE 0
#define RESP_FILE 1
#define RESP_CGI 2
#define RESP 2


#if (RESP == RESP_FILE)
	filedes = open("./2k_earth_daymap.jpg", O_RDONLY);
	if (filedes < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "open (file)");
	ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: image/jpg\r\nContent-Length: 463087\r\n\r\n");

	// filedes = open("./Kanan.mp3", O_RDONLY);
	// if (filedes < 0)
	// 	return WsLog::_errno(LVL_ERR, TGT_CONN, "open (file)");
	// ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: audio/mp3\r\nContent-Length: 14975750\r\n\r\n");
	
	// GENERAL QUESTION : send-buf-until-flushed (?)
	// one "send" per "epoll" (?)

	this->send(ostr); // UGLY : testing
	this->serv.ep.mod(this, EPOLLOUT);
	
#elif (RESP == RESP_CGI)

	err = this->exec_cgi();
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
		return (err);
	}
	

	// send (post) to cgi (?)
	// cgi-can-write :: flush BOYD - what is in Cgi::Conn::istr (?)

	ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n");
	this->send(ostr); // UGLY : testing
	this->ostr = std::string("");

	// ATTN : may have more to read ... 
	// to send to cgi-stdin
	this->serv.ep.mod(this, 0);
	
	// Resource::generate()
		// may fail here .. bad file, bad directory, bad pipe, not allowed
		
// the CONTENT_LENGTH environment variable needs to be set

// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read. 
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
	// wow : open cgi file and write TO stdin (?)
	

	// may get more body here 

	// need to know cgi INPUT fd .. to write to it (?)
	// OR : CGI .. "reads" from conn->istr
#else //  (RESP == RESP_SIMPLE)
// MINI_BODY
	ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 9\r\n\r\nGotcha!!\n");
	this->send(ostr); // not here .. but TEST 
	istr = std::string("");
	this->serv.ep.mod(this, EPOLLOUT);
#endif

	return (err);
}


// ∗ Just remember that, for chunked requests, your server needs to un-chunk
// them, the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. If no content_length is
// returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

int	Connection::pollout(void)
{
	int	err = 0;

	this->timeout();

#if (RESP == RESP_FILE)

	// and .. header (ostr) needs to be sent .. 
	// MsgBuf .. was pretty handy
	// output of CGI may be binary as well 
	if (filedes > 0)
	{
		err = read(filedes, this->ibuf, EPC_BUF_SIZ);
		if (err < 0)
		{
			close(filedes);
			filedes = -1;
			return WsLog::_errno(LVL_ERR, TGT_CONN, "read (file)");
		}
		if (err == 0)
		{
			close(filedes);
			filedes = -1;
			WsLog::_(LVL_DBG, TGT_CONN_RECV, "read: zero");
			return (-1);
		}
		err = this->send(ibuf, err);

		// and if NOT ALL BYTES ARE SENT (?)
		if (err < 0)
			return WsLog::_errno(LVL_ERR, TGT_CONN_SEND, "send");
		if (err == 0)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: zero");
			return (-1);
		}
		
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);

	}
#else

	// CGI : so .. we need to .. READY EVERYTHING FIRST ...
	
	// so that we can set the content-length header accordingly (?)
	// v.insert(v.end(), data2, data2 + strlen(data2));

	// CGI && SIMPLE .. "filled" (ostr)
	// Is this a good idea (?)
	if (ostr.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");
		this->serv.ep.mod(this, 0);
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
		return (-1);
	}
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
	// WsLog::_(LVL_DBG, TGT_CONN_SEND, ostr);

	
	if (ostr.size())
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, ostr);
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: all");
		
		// Q: have we sent (content-length) bytes (?)
		// Q: chunked send .. must close 
		// this->serv.ep.mod(this, 0);
		// nothing if that was the header for a FILE 

#if 0 // KEEP_ALIVE
		// see more HUP => read [0] with THIS .. AND NOT siege.conf
		this->serv.ep.mod(this, EPOLLIN); // something more here ..
#else
		// TEST : content-length header .. longer than what we send
		// curl: (18) end of response with 5 bytes missing
		return (-1);		
#endif
	}

#endif
	return (err); // (!) bytes written
}




// Resource .. built from Request
// CgiEnv   .. built from Request .. add to current ENV (?)

int	Connection::exec_cgi(void)
{
	int			err;

	std::string	path;
	std::string	file;
	
	if (false) // this->serv.get_port() == 8080)
	{
		path = std::string("/usr/bin/python");
		file = std::string("test.py");
	}
	else
	{
		// path = std::string("/usr/bin/perl");
		// file = std::string("test.pl");

		path = std::string("/usr/bin/php-cgi"); // DIFFERENT
		file = std::string("test.php");
	}

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
		// cleanup
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}
	if (pid == 0)
	{
		// Epoll:: CLOEXEC should have closed Epoll::epfd

		close(p1[1]);
		err = dup2(p1[0], STDIN_FILENO);
		if (err < 0)
		{
			// cleanup
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p1[0]);
		
		close(p2[0]);
		err = dup2(p2[1], STDOUT_FILENO);
		if (err < 0)
		{
			// cleanup
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p2[1]);

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
		// cgienv.add("CONTENT_TYPE", "multipart/form-data"); // NEEDS BOUNDARY

		val = this->header("Content-type");
		if (val.size())
			cgienv.add("CONTENT_TYPE", val.c_str());

		// wait to read this much -- even if GET (?) NOPE
		// cgienv.add("CONTENT_LENGTH", this->istr.size()); // body
		val = this->header("Content-length");
		if (val.size())
			cgienv.add("CONTENT_LENGTH", val.c_str());

// /home/kdonlon/Documents/Projects/webserv/git/src

		cgienv.add("PATH_INFO", "path info"); // added to PHP_SELF (?)
// PATH_TRANSLATED
// Maps the script's virtual path to the physical path used to call the script. This is done by taking any PATH_INFO component of the request URI and performing any virtual-to-physical translation appropriate.

		cgienv.add("SCRIPT_NAME", "test.php");
		cgienv.add("SCRIPT_FILENAME", "test.php");
// 		SCRIPT_NAME
// Returns the part of the URL from the protocol name up to the query string in the first line of the HTTP request.
		
		cgienv.add("QUERY_STRING", "g1=get-one&g2=get-two");
		
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
		cgienv.add("SERVER_NAME", "webserv");
		cgienv.add("SERVER_PORT", "8080");
		cgienv.add("SERVER_PROTOCOL", "HTTP/1.1");
		cgienv.add("SERVER_SOFTWARE", "webserv");


		// so .. read FROM cgi .. until (ostr) is full
		// then we can slap the headers on (?)
		const char **envp = cgienv.gen();

		err = execve(args[0], (char* const*) args, (char* const*) envp);
		std::cout << "\n\nwtf\n\n";
		return (err);	
	}		
	
	WsLog::_(LVL_INFO, TGT_CONN, "exec cgi");

	close(p1[0]);
	close(p2[1]);

	int			cgifd;
	EpollClient	*epc_cgi;
	
	cgifd = dup(p1[1]);
	if (cgifd < 0)
	{
		// cleanup (?)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	}
	close (p1[1]);

	epc_cgi = new CgiPipe(cgifd, *this);
	err = this->serv.ep.add(epc_cgi, EPOLLOUT);
	if (err < 0)
	{
		// cleanup (?)
		// delete (epc_cgi);
		return (err);
	}


	cgifd = dup(p2[0]);
	if (cgifd < 0)
	{
		// cleanup (?)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	}
	close (p2[0]);
	
	epc_cgi = new CgiPipe(cgifd, *this);
	err = this->serv.ep.add(epc_cgi, EPOLLIN);
	if (err < 0)
	{
		// cleanup (?)
		// delete (epc_cgi);
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

