/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/06 23:11:51 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "Connection.hpp"
# include "CgiPipe.hpp"
# include "FcgiConn.hpp"
# include "bridge.hpp"


enum
{
	RSRC_RESP_INIT = 0,
	RSRC_RESP_HEAD,
	RSRC_RESP_BODY,
	RSRC_RESP_DONE,
	RSRC_RESP_ERR
};

# ifndef RSRC_FCGI
#  define RSRC_FCGI 1
# endif

class ResourceCgi : public Resource
{
private:
	ResourceCgi				 (const ResourceCgi & ) {}
	ResourceCgi & operator = (const ResourceCgi & ) { return (*this); }
public:
	ResourceCgi(void) :  
		hed(0), 
		hlen(0), 
		clen(0), 
		tlen(0),
		ka(0), 
		error(0),

		pid(0), 
		ip(NULL), 
		op(NULL),
		stat(-1),
		xit(-1), 
		sig(-1)
	{}
	~ResourceCgi();

	int			init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn);
	int			init(Epoll *ep, CgiEnv *cgienv, Connection *conn);
	
// SHARED
	int			recv_data(char *buf, int siz);
	int			chk_rsp_hed(std::string & ostr);
	int			consumed(int bytes);
	void		set_err(int e);

	
	int			hed;
	int			hlen;
	int			clen;
	int			tlen;
	int			ka;
	int			error;
	

// CgiPipe
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
	std::string	ostr;

	int			stat;
	int			xit;
	int			sig;
	
	FcgiPipe	*fcgi;
// virtual
	void        push_body(void);
	int			status(void);
	void		conn_closed(void);
	int			wait(int opt);

	int			rem(EpollClient *epc);
private:
	Connection	*conn;
};


#endif