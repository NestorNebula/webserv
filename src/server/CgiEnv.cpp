/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 11:06:43 by kdonlon          ###   ########.fr       */
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

// from_headers
// plus (webserv)
void	CgiEnv::add(const char *key, const char *val)
{
	// <map> first .. to override multiple (?)
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
	// Maps the script's virtual path to the physical path used to call the script. This is done by taking any PATH_INFO component of the request URI and performing any virtual-to-physical translation appropriate.
	// SCRIPT_NAME
	// Returns the part of the URL from the protocol name up to the query string in the first line of the HTTP request.

int     CgiEnv::from_conn(Connection & conn, std::string & file)
{
	std::string val;
	
	this->add("REQUEST_METHOD", "GET");  // conn->method
	// this->add("REQUEST_METHOD", "POST"); 
	this->add("QUERY_STRING", "g1=get-one&g2=get-two");

	this->add("PATH_INFO", "path info"); // added to PHP_SELF (?)
	this->add("SCRIPT_NAME", file.c_str());
		// PHP CGI depends on non-standard SCRIPT_FILENAME
	this->add("SCRIPT_FILENAME", file.c_str()); // (php)
	
// php-cgi: This PHP CGI binary was compiled with force-cgi-redirect enabled.  This
// means that a page will only be served up if the REDIRECT_STATUS CGI variable is set
	this->add("REDIRECT_STATUS", "1");
// Python: legacy-cgi
	this->add("PYTHONPATH", 
		"/home/kdonlon/Documents/Projects/webserv/legacy-cgi-main/");

	val = conn.header("Host");
	if (val.size())
		this->add("HTTP_HOST", val.c_str());
	val = conn.header("User-Agent");
	if (val.size())
		this->add("HTTP_USER_AGENT", val.c_str());
	val = conn.header("Accept");
	if (val.size())
		this->add("HTTP_ACCEPT", val.c_str());
	this->add("HTTP_COOKIE", "chocolate chip");
	
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
	
	this->add("REMOTE_ADDR", addr_2_str(&conn.addr).c_str());
	// this->add("REMOTE_HOST", "remote host");
	// this->add("REMOTE_USER", "remote user");
	
	this->add("SERVER_NAME", "webserv"); // virtual
	this->add("SERVER_PORT", conn.serv.get_port());
	this->add("SERVER_PROTOCOL", "HTTP/1.1");
	this->add("SERVER_SOFTWARE", "webserv");

// In addition to these, the header lines recieved from the client, if any, are placed into the environment with the prefix HTTP_ followed by the header name. Any - characters in the header name are changed to _ characters. The server may exclude any headers which it has already processed, such as Authorization, Content-type, and Content-length. If necessary, the server may choose to exclude any or all of these headers if including them would exceed any system environment limits. 

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
