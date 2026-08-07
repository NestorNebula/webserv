/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:29 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/07 12:54:30 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BRIDGE_HPP
# define BRIDGE_HPP

# include <string>
# include <sstream>
# include <algorithm>
# include <cctype>
# include <CgiPipe.hpp>
# include <WsLog.hpp>

# define REQ_INIT 0
# define REQ_READ_HEAD 1
# define REQ_HAVE_HEAD 2
# define REQ_READ_BODY 3
# define REQ_HAVE_BODY 4
# define REQ_DONE 5


std::string hedval_str(std::string & str, const char *key);

class Request
{
public:
	Request(void) : state(REQ_INIT), blen(0), clen(0), chnk(0) {}
	~Request() {}

	int         push_data(const char *buf, size_t siz);
	int			body_stat(void);
	int         init(void);
	std::string header(const char *key) const;

	int         get_state(void) const { return this->state; }
	std::string &get_body(void) { return this->body; }
	std::string &get_fext(void) { return this->fext; }
	
	void		reset(void);

// private:
	int			state;
	
	std::string	head;
	std::string body; // sess.ip_data
	std::string exec;

private:
	size_t		blen;
	size_t		clen;
	int			chnk;

	std::string meth;
	std::string path;
	std::string file;
	std::string fext;
	std::string vars;

};

class Resource
{
public:
	virtual ~Resource() {}
	virtual void	push_body(void) = 0;
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
  
	// fill_input
	int write(const char *buf, size_t siz)
	{
		int err = this->req.push_data(buf, siz);
		if (err < REQ_HAVE_HEAD)
			return (err);
		return (err);
	}
	int		pull_data(std::string & dst)
	{
		(void)dst;
		return (0);
	}
	void	reset(void)
	{
		this->req.reset();	
	}
	int req_state(void) { return this->req.get_state(); }
};

#endif