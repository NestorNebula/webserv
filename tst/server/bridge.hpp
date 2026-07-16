/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:29 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/16 20:29:02 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BRIDGE_HPP
# define BRIDGE_HPP

# include <string>
# include <sstream>
# include <algorithm>
# include <CgiPipe.hpp>
# include <WsLog.hpp>

# define REQ_INIT 0
# define REQ_READ_HEAD 1
# define REQ_HAVE_HEAD 2
# define REQ_READ_BODY 3
# define REQ_HAVE_BODY 4
# define REQ_DONE 5

class Request
{
public:
	Request(void) : state(REQ_INIT) {}

	int         push_data(const char *buf, size_t siz);
	int         init(void);
	std::string header(const char *key) const;

	int         get_state(void) const { return this->state; }
	std::string &get_body(void) { return this->body; }
	std::string &get_fext(void) { return this->fext; }
	
private:
	int			state;
	
	std::string	head;
	std::string body; // sess.ip_data
	std::string exec;

	std::string meth;
	std::string path;
	std::string file;
	std::string fext;
	std::string vars;
};

class Resource
{
public:

	std::string	ostr;
};

class ResourceCgi : public Resource
{
public:
	pid_t			cgi_pid;
	CgiPipe			*cgi_ip;
	CgiPipe			*cgi_op;
};

class Session
{
public:
	Session(void) : res(NULL) {}
	~Session()
	{
		if (this->res)
			delete (this->res);
	}
	Request     req;
	Resource    *res;
  
	int push_data(const char *buf, size_t siz)
	{
		int err = this->req.push_data(buf, siz);
		if (err < REQ_HAVE_HEAD)
			return (err);
		// res.push_data(body)
		// have head : make rsrc
		// conn->exec_cgi()
		// tell (res) data is available
		
		return (err);
	}
	int req_state(void) { return this->req.get_state(); }
};

#endif