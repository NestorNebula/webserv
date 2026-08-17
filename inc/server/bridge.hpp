/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:29 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/17 14:28:53 by kdonlon          ###   ########.fr       */
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

#if 0
std::string hedval_str(std::string & str, const char *key);

class br_Request
{
public:
	br_Request(void) : state(REQ_INIT), blen(0), clen(0), chnk(0) {}
	~br_Request() {}

	int         push_data(const char *buf, size_t siz);
	int			body_stat(void);
	int         init(void);
	std::string header(const char *key) const;

	int         get_state(void) const { return this->state; }
	std::string &get_body(void) { return this->body; }
	const std::string &get_fext(void) const { return this->fext; }
	
	void		reset(void);

// private:
	int				state;
	
	std::string		head;
	std::string 	body;
	std::string 	exec;

private:
	size_t			blen;
	size_t			clen;
	int				chnk;

	std::string 	meth;
	std::string 	path;
	std::string 	file;
	std::string 	fext;
	std::string 	vars;

};

class br_Session
{
public:
	br_Session(void) {}
	~br_Session() {}
	
	br_Request     req;
  
// WEBSERV : REQUEST (body)
  	const br_Request &getRequest() const
		{ return (this->req); }
	int write(const char *buf, size_t siz)
	{
		int err = this->req.push_data(buf, siz);
		if (err < REQ_HAVE_HEAD)
			return (err);
		return (err);
	}
	void	reset(void)
	{
		this->req.reset();	
	}
	int req_state(void) { return this->req.get_state(); }
};
#endif
#endif