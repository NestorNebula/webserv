/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/14 15:54:44 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiEnv.hpp"
#include "Connection.hpp"
#include "Server.hpp"

CgiEnv::CgiEnv(void) : res(NULL)
{

}

CgiEnv::~CgiEnv()
{
	if (res)
		delete[] res;
}

// like this .. we store the backing data for envp
void	CgiEnv::add(const char *key, const char *val)
{
	// <map> first [keky]
	data.push_back(std::string(key) + std::string("=") + std::string(val));
}

void	CgiEnv::add(const char *key, int n)
{
	data.push_back(std::string(key) + std::string("=") + num_2_str(n));
}

// From the meta-variables thus generated, a URI, the 'Script-URI', can
//    be constructed.  This MUST have the property that if the client had
//    accessed this URI instead, then the script would have been executed
//    with the same values for the SCRIPT_NAME, PATH_INFO and QUERY_STRING
//    meta-variables.

// script-URI = <scheme> "://" <server-name> ":" <server-port>
//                    <script-path> <extra-path> "?" <query-string>
		// PATH_TRANSLATED
	// Maps the script's virtual path to the physical path used to call the script. 
	// This is done by taking any PATH_INFO component of the request URI and performing any virtual-to-physical translation appropriate.
	// SCRIPT_NAME
	// Returns the part of the URL from the protocol name up to the query string in the first line of the HTTP request.

int     CgiEnv::from_conn(Connection & conn, std::string & file)
{
	std::string val;
	
	val = conn.header("METH");
	if (val.size())
		this->add("REQUEST_METHOD", val.c_str());
	else
		this->add("REQUEST_METHOD", "GET");
	val = conn.header("PATH");
	if (val.size())
		this->add("_PATH", val.c_str());
	val = conn.header("QUERY");
	if (val.size())
		this->add("QUERY_STRING", val.c_str());
	else
		this->add("QUERY_STRING", "g1=get-one&g2=get-two");

// (cwd) !!!
	this->add("PATH_INFO", "path info"); // added to PHP_SELF (?)
	this->add("SCRIPT_NAME", file.c_str());
		// PHP CGI depends on non-standard SCRIPT_FILENAME
	this->add("SCRIPT_FILENAME", file.c_str()); // (php)

	
	
// php-cgi: This PHP CGI binary was compiled with force-cgi-redirect enabled.
// This means that a page will only be served up 
// if the REDIRECT_STATUS CGI variable is set
	this->add("REDIRECT_STATUS", "1");
	this->add("PYTHONPATH", 
		"/home/kdonlon/Documents/Projects/webserv/legacy-cgi-main/");


// If the output of a form is being processed, check that CONTENT_TYPE
// is "application/x-www-form-urlencoded"
// or "multipart/form-data".
// If CONTENT_TYPE is blank, the script can reject the request
// with a 415 'Unsupported Media Type' error, where supported by the
// protocol.

	val = conn.header("Content-type");
	if (val.size())
		this->add("CONTENT_TYPE", val.c_str());
	val = conn.header("Content-length");
	if (val.size())
		this->add("CONTENT_LENGTH", val.c_str());
		
// In addition to these, the header lines recieved from the client, if any, are placed into the environment with the prefix HTTP_ followed by the header name. Any - characters in the header name are changed to _ characters. The server may exclude any headers which it has already processed, such as Authorization, Content-type, and Content-length. If necessary, the server may choose to exclude any or all of these headers if including them would exceed any system environment limits. 
	val = conn.header("Host");
	if (val.size())
		this->add("HTTP_HOST", val.c_str());
	val = conn.header("User-Agent");
	if (val.size())
		this->add("HTTP_USER_AGENT", val.c_str());
	val = conn.header("Accept");
	if (val.size())
		this->add("HTTP_ACCEPT", val.c_str());
	val = conn.header("Accept-Language");
	if (val.size())
		this->add("HTTP_ACCEPT_LANGUAGE", val.c_str());
	val = conn.header("Accept-Encoding");
	if (val.size())
		this->add("HTTP_ACCEPT_ENCODING", val.c_str());
	val = conn.header("Connection");
	if (val.size())
		this->add("HTTP_CONNECTION", val.c_str());
		// Upgrade-Insecure-Requesets
		// Sec-Fetch
	this->add("HTTP_COOKIE", "chocolate chip");
	
	this->add("REMOTE_ADDR", addr_2_str(&conn.addr).c_str());
	// this->add("REMOTE_HOST", "remote host");
	// this->add("REMOTE_USER", "remote user");
	
	this->add("SERVER_NAME", "webserv"); // virtual
	this->add("SERVER_PORT", conn.serv.get_port());
	this->add("SERVER_PROTOCOL", "HTTP/1.1");
	this->add("SERVER_SOFTWARE", "webserv");

    return (0);
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


