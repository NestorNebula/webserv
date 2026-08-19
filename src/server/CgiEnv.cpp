/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/19 10:48:24 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiEnv.hpp"
#include "Connection.hpp"
#include "Server.hpp"
#include "helpers.hpp"
#include "http_utils.hpp"
#include "helpers.hpp"
#include "http_utils.hpp"

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
	this->kv[ std::string(key) ] = std::string(val);
}

void	CgiEnv::add(const char *key, int n)
{
	this->kv[ std::string (key) ] = num_2_str(n);
	this->kv[ std::string (key) ] = num_2_str(n);
}

std::string & CgiEnv::get(const char *key)
{
	return (this->kv[ std::string (key) ]);
}

// could return HTTP_ERRNO
int     CgiEnv::from_conn(Connection & conn)
{
	this->kv.clear();

	Session &sess = conn.sess;
	Request &req  = sess.getRequest();
	const Headers &headers = req.getHeaders();

	if (!req.hasMethod())
	{
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "METHOD not set");
		conn.set_err(400); // Bad Request
		return (-1);
	}
	if (!req.hasURL())
	{
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "URL not set");
		conn.set_err(400); // Bad Request
		return (-1);
	}
	
	info = sess.getCgiInfo();
	
	this->add("REQUEST_METHOD", methodToString(req.getMethod()).c_str());

	// std::cerr << "METH  : " << methodToString(req.getMethod()) << std::endl;
	// std::cerr << " URL  : " << info.scriptPath << std::endl;
	// std::cerr << "SERV  : root : " << conn.serv.get_conf().root << std::endl;

	std::string path_rel = conn.serv.get_conf().root + info.scriptPath;
	script.parse(path_rel);
	// script.dump();

	if (access(script.path.c_str(), F_OK))
	{
		WsLog::_(LVL_DBG, TGT_CGI_ENV, "access: ", script.path);
		conn.set_err(404); // File Not Found
		return (-1);
	}
	
	this->add("CWD", script.fldr.c_str()); 
	
	if (script.fext == std::string(".php"))
	{
		lang = CGI_PHP;
// php-cgi: This PHP CGI binary was compiled with force-cgi-redirect enabled.
// This means that a page will only be served up 
// if the REDIRECT_STATUS CGI variable is set
		this->add("DOCUMENT_ROOT", script.fldr.c_str());
		this->add("SCRIPT_NAME", script.file.c_str());
		this->add("SCRIPT_FILENAME", script.path.c_str());

		this->add("REDIRECT_STATUS", "1");		
	}
	else if (script.fext == std::string(".py"))
	{
		lang = CGI_PYTHON;
		// how does HOME work without this ?
		this->add("PYTHONPATH", conn.serv.pycgi.c_str());
	}
	else if (script.fext == std::string(".pl"))
	{
		lang = CGI_PERL;
	}
	else
	{
		WsLog::_(LVL_ERR, TGT_CGI_ENV, "EXEC not set");
		conn.set_err(403); // Forbidden
		return (-1);
	}
	this->args[0] = info.executablePath.c_str();
	this->args[1] = script.file.c_str();
	this->args[2] = NULL;	
	
	if (req.hasQuery())
	{
		this->add("QUERY_STRING", req.getQuery().c_str());
	}
// If the output of a form is being processed, check that CONTENT_TYPE
// is "application/x-www-form-urlencoded"
// or "multipart/form-data".
// If CONTENT_TYPE is blank, the script can reject the request
// with a 415 'Unsupported Media Type' error, where supported by the
// protocol.
	if (req.hasHeader("Content-type"))
		this->add("CONTENT_TYPE", headers.find("Content-type")->second.c_str());
	if (req.hasHeader("Content-length"))
		this->add("CONTENT_LENGTH", headers.find("Content-length")->second.c_str());
	


	Headers::const_iterator hit = headers.begin();
	while (hit != headers.end())
	{
		std::string hk = hit->first;
		std::string hv = hit->second;
		hk.insert(0, "http_");
		header_key(hk);
		this->add(hk.c_str(), hv.c_str());
		hit++;
	}
	
	this->add("REMOTE_ADDR", conn.get_addr().c_str());
	// this->add("REMOTE_HOST", "remote host");
	// this->add("REMOTE_USER", "remote user");
	
	this->add("SERVER_NAME", "webserv");
	this->add("SERVER_PORT", conn.serv.get_port());
	this->add("SERVER_PROTOCOL", "HTTP/1.0");
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
		// WsLog::color(WSL_GREEN);
		// WsLog::_(LVL_DBG, TGT_CGI_ENV, "(kv) : ", it->c_str());
		// WsLog::color(WSL_GREEN);
		// WsLog::_(LVL_DBG, TGT_CGI_ENV, "(kv) : ", it->c_str());
		*ins++ = it->c_str();
		it++;
	}
	*ins = NULL;
	return (res);
}

// FCGI
// FCGI