/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/10 12:02:00 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiEnv.hpp"
#include "Connection.hpp"
#include "Server.hpp"

CgiEnv::CgiEnv(void) : lang (0), res (NULL)
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

std::string & CgiEnv::get(const char *key)
{
	return (this->kv[ std::string (key) ]);
}


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
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "METHOD not set");
		return (-1);
	}

	// val = req.header("PATH");
	// if (val.size())
	// 	this->add("_PATH", val.c_str());
	
	// file = req.header("FILE");
	file = req.header("PATH");
	if (!file.size())
	{
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "FILE : not set");
		return (-1);
	}
	
	path = conn.serv.data_root + file;
	
	this->add("SCRIPT_NAME", path.c_str());
	
	if (access(path.c_str(), F_OK))
	{
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "FILE : does not exist");
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "PATH\n", path);
		return (-1);
	}

	size_t pos = path.find_last_of("/");
	std::string cwd = path.substr(0, pos);
	this->add("CWD", cwd.c_str());	

	// WsLog::color(WSL_GREEN);
	// WsLog::_(LVL_DBG, TGT_CGI_ENV, "cwd : ", cwd);
	
	this->add("SCRIPT_FILENAME", path.c_str());	

	std::string &fext = req.get_fext();
	if (fext == std::string("php"))
	{
		lang = CGI_PHP;
		exec = conn.serv.bin_php;
// php-cgi: This PHP CGI binary was compiled with force-cgi-redirect enabled.
// This means that a page will only be served up 
// if the REDIRECT_STATUS CGI variable is set
		this->add("REDIRECT_STATUS", "1");		
	}
	else if (fext == std::string("py"))
	{
		lang = CGI_PYTHON;
		exec = conn.serv.bin_py;
		// this->add("PYTHONPATH", conn.serv.pycgi.c_str());
	}
	else if (fext == std::string("pl"))
	{
		lang = CGI_PERL;
		exec = conn.serv.bin_pl;
	}
	else
	{
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "EXEC not set");
		std::cerr << req.head;
		return (-1);
	}
	this->args[0] = this->exec.c_str();
	this->args[1] = path.c_str();
	this->args[2] = NULL;	
	
	
	val = req.header("VARS");
	if (val.size())
		this->add("QUERY_STRING", val.c_str());
		
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

// FCGI