/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/04 18:45:53 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "Connection.hpp"
# include "CgiPipe.hpp"
# include "bridge.hpp"


enum
{
	RSRC_RESP_INIT = 0,
	RSRC_RESP_HEAD,
	RSRC_RESP_BODY,
	RSRC_RESP_DONE,
	RSRC_RESP_ERR
};

class ResourceCgi : public Resource
{
private:
	ResourceCgi				 (const ResourceCgi & ) {}
	ResourceCgi & operator = (const ResourceCgi & ) { return (*this); }
public:
	ResourceCgi(void) : pid(0), ip(NULL), op(NULL), 
	hed(0), 
	hlen(0), 
	clen(0), 
	tlen(0),
	ka(0), 
	error(0),
	stat(-1),
	xit(-1), 
	sig(-1)
	{}
	~ResourceCgi();

	int			init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn);
	
	int			recv_data(char *buf, int siz);
	void        push_body(void);
    
	// shared
	int			chk_rsp_hed(std::string & ostr);
	
	void		set_err(int e);
	
//
	// FcgiConn
	// CgiFast		*fcgi;
	
// CgiPipe
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
	std::string	ostr;

// some of these could be good for FCGI as well .. 

	int			hed;
	int			hlen;
	int			clen;
	int			tlen;
	int			ka;
	int			error;
	
// CgiPipe
	int			stat;
	int			xit;
	int			sig;

// both : over-ride
	void		conn_closed(void);
	int			status(int opt);

	int			rem(CgiPipe *epc);
private:
	Connection	*conn;
};


#endif