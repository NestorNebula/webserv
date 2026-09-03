/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:47:07 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 20:57:09 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiEnv.hpp"
#include "Connection.hpp"
#include "Server.hpp"
#include "helpers.hpp"

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
	this->kv[ std::string (key) ] = toString(n);
}

std::string & CgiEnv::get(const char *key)
{
	return (this->kv[ std::string (key) ]);
}


static void header_key(std::string &s) 
{
	for (std::string::iterator it = s.begin(), ite = s.end(); it != ite; it++) 
	{
		if (*it == '-')
			(*it = '_');
		else
			*it = std::toupper(static_cast<unsigned char>(*it));
	}
}

int     CgiEnv::from_conn(Connection & conn)
{
	this->kv.clear();

	Session &sess = conn.sess;
	Request &req  = sess.getRequest();
	const Headers &headers = req.getHeaders();

	ServerConfig	&conf = conn.serv.get_conf();

	if (!req.hasMethod())
	{
		WSLOG(LVL_ERR, TGT_CGI_ENV, "METHOD not set");
		return (conn.set_err(400)); // Bad Request
	}
	if (!req.hasURL())
	{
		WSLOG(LVL_ERR, TGT_CGI_ENV, "URL not set");
		return (conn.set_err(400)); // Bad Request
	}
	
	info = sess.getCgiInfo();
	
	this->add("REQUEST_METHOD", methodToString(req.getMethod()).c_str());

	std::string path_rel = conf.root + info.scriptPath;
	script.parse(path_rel);

	// WSLOG(LVL_DBG, TGT_CGI_ENV, "script: ", script.path);
	// this should have been checked before we got here
	if (access(script.path.c_str(), F_OK | R_OK))
	{
		WSLOG(LVL_DBG, TGT_CGI_ENV, "access: ", script.path);
		return (conn.set_err(404)); // File Not Found
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
		if (!conf.pycgi_dir.size())
		{
			WSLOG(LVL_ERR, TGT_CGI_ENV, "bad  : pycgi_dir");
			return (conn.set_err(500));
		}
		lang = CGI_PYTHON;
		std::string pyrel = conf.root + conf.pycgi_dir;
		FilePath pypath(pyrel);
		this->add("PYTHONPATH", pypath.path.c_str());
	}
	else if (script.fext == std::string(".pl"))
	{
		lang = CGI_PERL;
	}
	else
	{
		WSLOG(LVL_ERR, TGT_CGI_ENV, "EXEC not set");
		return (conn.set_err(403)); // Forbidden
	}
	this->args[0] = info.executablePath.c_str();
	this->args[1] = script.file.c_str();
	this->args[2] = NULL;	
	
	if (req.hasQuery())
	{
		this->add("QUERY_STRING", req.getQuery().c_str());
	}
	if (req.hasHeader("Content-Type"))
		this->add("CONTENT_TYPE", headers.find("Content-type")->second.c_str());
	else if (req.getMethod() == METHOD_POST)
	{
		// If the output of a form is being processed, check that CONTENT_TYPE
		// is "application/x-www-form-urlencoded"
		// or "multipart/form-data".
		// If CONTENT_TYPE is blank, the script can reject the request 
		// with a 415 'Unsupported Media Type' error, where supported by the protocol.
		WSLOG(LVL_ERR, TGT_CGI_ENV, "missing : content-type");
		return (conn.set_err(415)); // Unsupported Media Type
		
	}
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
	// SERVER_ADDR
	this->add("SERVER_PORT", conn.serv.get_port());
	this->add("SERVER_PROTOCOL", "HTTP/1.0"); // conn.serv (?)
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
		// WSCOL(WSL_GREEN);
		// WSLOG(LVL_DBG, TGT_CGI_ENV, "(kv) : ", it->c_str());
		// WSCOL(WSL_GREEN);
		// WSLOG(LVL_DBG, TGT_CGI_ENV, "(kv) : ", it->c_str());
		*ins++ = it->c_str();
		it++;
	}
	*ins = NULL;
	return (res);
}