/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/21 17:46:38 by kdonlon          ###   ########.fr       */
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

void	CgiEnv::add(const char *key, const char *val)
{
	this->kv[ std::string(key) ] = std::string(val);
}

void	CgiEnv::add(const char *key, int n)
{
	this->kv[ std::string (key) ] = num_2_str(n);
}

// From the meta-variables thus generated, a URI, the 'Script-URI', can
//    be constructed.  This MUST have the property that if the client had
//    accessed this URI instead, then the script would have been executed
//    with the same values for the SCRIPT_NAME, PATH_INFO and QUERY_STRING
//    meta-variables.

// script-URI = 
// <scheme> "://" <server-name> ":" <server-port>
//<script-path> <extra-path> "?" <query-string>

		// PATH_TRANSLATED
// Maps the script's virtual path to the physical path used to call the script. 
// This is done by taking any PATH_INFO component of the request URI and performing any virtual-to-physical translation appropriate.

// SCRIPT_NAME
// Returns the part of the URL from the protocol name up to the query string in the first line of the HTTP request.

int     CgiEnv::from_conn(Connection & conn)
{
	Request &req = conn.sess.req;

	this->kv.clear();
	
	std::string val;

	val = req.header("METH");
	if (val.size())
		this->add("REQUEST_METHOD", val.c_str());
	else
	{
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "METHOD not set");
		return (-1);
	}

	val = req.header("PATH");
	if (val.size())
	{
		this->add("_PATH", val.c_str()); // NB : relative 
// PATH_INFO
// The extra path information, as given by the client. 
// In other words, scripts can be accessed by their virtual pathname, 
// followed by extra information at the end of this path.
// The extra information is sent as PATH_INFO.
// This information should be decoded by the server 
// if it comes from a URL before it is passed to the CGI script.
// http://example.com/cgi-bin/printenv.pl/with/additional/path?and=a&query=string
// If a slash and additional directory name(s) are appended to the URL immediately after the name of the script (in this example, /with/additional/path), then that path is stored in the PATH_INFO environment variable before the script is called. 
		// this->add("PATH_INFO", val.c_str());
// PATH_TRANSLATED
// The server provides a translated version of PATH_INFO, 
// which takes the path and does any virtual-to-physical mapping to it. 		
	}
	
// SESSION / REQUEST - absolute path (?) (chdir) ? 
// kd : I'm not 100% sure what I need here .. absolute, relative, relative-to-what
	file = req.header("FILE");
	// so .. relative to .. PWD -- the folder in which webserv was launched
	// test : upload : writes to "./" .. 
	// which should depend on where the script is 
	// php (./) would expect .. to write in the same folder as the script
	// NOT .. the PWD -- where the application was launched 
	// file = std::string("tst/server/") + file;
	if (file.size())
	{
// SCRIPT_NAME
// A virtual path to the script being executed, used for self-referencing URLs.
		this->add("SCRIPT_NAME", file.c_str());
			// PHP CGI depends on non-standard SCRIPT_FILENAME
		this->add("SCRIPT_FILENAME", file.c_str());	
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "FILE not set");
		return (-1);
	}
	
// SESSION / REQUEST / CONFIG : exec_path
// kd : I imagine this could be set during the parsing of the request/config
	std::string &fext = req.get_fext();
	if (fext == std::string("php"))
	{
		exec = std::string("/usr/bin/php-cgi"); 
		this->args[0] = this->exec.c_str();
		this->args[1] = file.c_str();
		this->args[2] = NULL;		
	}
	else if (fext == std::string("py"))
	{
		exec = std::string("/usr/bin/python"); 
		this->args[0] = this->exec.c_str();
		this->args[1] = file.c_str();
		this->args[2] = NULL;		
	}
	else if (fext == std::string("pl"))
	{
		exec = std::string("/usr/bin/perl"); 
		this->args[0] = this->exec.c_str();
		this->args[1] = file.c_str();
		this->args[2] = NULL;		
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "EXEC not set");
		return (-1); // ERROR (!)
	}
// SESSION
	val = req.header("VARS");
	if (val.size())
		this->add("QUERY_STRING", val.c_str());
		
// php-cgi: This PHP CGI binary was compiled with force-cgi-redirect enabled.
// This means that a page will only be served up 
// if the REDIRECT_STATUS CGI variable is set
	this->add("REDIRECT_STATUS", "1");
	
// SERVER / CONFIG : server_root/pycgi
	this->add("PYTHONPATH", 
		"/home/kdonlon/Documents/Projects/webserv/git/pycgi/");
		// "/media/kdonlon/data/Documents/42/webserv/git/pycgi/");


// If the output of a form is being processed, check that CONTENT_TYPE
// is "application/x-www-form-urlencoded"
// or "multipart/form-data".
// If CONTENT_TYPE is blank, the script can reject the request
// with a 415 'Unsupported Media Type' error, where supported by the
// protocol.

	val = req.header("Content-type");
	if (val.size())
		this->add("CONTENT_TYPE", val.c_str());
	val = req.header("Content-length");
	if (val.size())
		this->add("CONTENT_LENGTH", val.c_str());
		
// In addition to these, the header lines recieved from the client, if any, are placed into the environment with the prefix HTTP_ followed by the header name. Any - characters in the header name are changed to _ characters. The server may exclude any headers which it has already processed, such as Authorization, Content-type, and Content-length. If necessary, the server may choose to exclude any or all of these headers if including them would exceed any system environment limits. 

	val = req.header("Host");
	if (val.size())
		this->add("HTTP_HOST", val.c_str());
	val = req.header("Referer");
	if (val.size())
		this->add("HTTP_REFERER", val.c_str());
	val = req.header("User-Agent");
	if (val.size())
		this->add("HTTP_USER_AGENT", val.c_str());

	val = req.header("Transfer-Encoding");
	if (val.size())
		this->add("HTTP_TRANSFER_ENCODING", val.c_str());

	val = req.header("Accept");
	if (val.size())
		this->add("HTTP_ACCEPT", val.c_str());
	val = req.header("Accept-Encoding");
	if (val.size())
		this->add("HTTP_ACCEPT_ENCODING", val.c_str());
	val = req.header("Accept-Language");
	if (val.size())
		this->add("HTTP_ACCEPT_LANGUAGE", val.c_str());
	val = req.header("Connection");
	if (val.size())
		this->add("HTTP_CONNECTION", val.c_str());
// SERVER		
	this->add("HTTP_COOKIE", "chocolate chip");
	
	this->add("REMOTE_ADDR", conn.get_addr().c_str());
	// this->add("REMOTE_HOST", "remote host");
	// this->add("REMOTE_USER", "remote user");
	
// SERVER
	this->add("SERVER_NAME", "webserv");
	this->add("SERVER_PORT", conn.serv.get_port());
	this->add("SERVER_PROTOCOL", "HTTP/1.1");
	this->add("SERVER_SOFTWARE", "webserv");
	
	this->add("GATEWAY_INTERFACE", "CGI/1.0");

    return (0);
}

const char	**CgiEnv::gen(void)
{
	this->data.clear();
	if (res)
		delete[] res;

	std::map<std::string, std::string>::iterator kvit = kv.begin();
	while (kvit != kv.end())
	{
		data.push_back(std::string(kvit->first) + std::string("=") + std::string(kvit->second));
		kvit++;
	}
	
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
