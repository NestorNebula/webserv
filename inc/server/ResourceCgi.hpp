/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/18 15:15:03 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "Connection.hpp"
# include "CgiPipe.hpp"
# include "FcgiPipe.hpp"

enum
{
	RSRC_RESP_INIT = 0,
	RSRC_RESP_HEAD,
	RSRC_RESP_BODY,
	RSRC_RESP_DONE,
	RSRC_RESP_ERR
};

class ResourceCgi
{
public:
	ResourceCgi(void) :  
		hed(0),
		error(0),
		conn(NULL)
	{}
	virtual ~ResourceCgi() {};

	int				recv_data(char *buf, int siz);
	int				chk_rsp_hed(std::string & ostr);
	void			set_err(int e);
	std::string &	get_resp(void) { return (this->resp); }
	
	int				hed; // state
	int				error;
	std::string		resp;
	
	virtual void	push_body(void) = 0;
	virtual int		status(void) = 0;
	virtual void	conn_closed(void) = 0;
	virtual int		wait(int opt) = 0;

	virtual int		rem(EpollClient *epc) = 0;
protected:
	Connection		*conn;
};


class ResourceFcgi : public ResourceCgi
{
private:
	ResourceFcgi			  (const ResourceFcgi & that ) : ResourceCgi(that) {}
	ResourceFcgi & operator = (const ResourceFcgi & ) { return (*this); }
public:
	ResourceFcgi(void) : ResourceCgi(), fcgi(NULL) {}
	~ResourceFcgi();

	int			init(Epoll *ep, CgiEnv *cgienv, Connection *conn);

	void        push_body(void);
	int			status(void);
	void		conn_closed(void);
	int			wait(int opt);

	int			rem(EpollClient *epc);
private:
	FcgiPipe	*fcgi;
};


class ResourcePiped : public ResourceCgi
{
private:
	ResourcePiped				 (const ResourcePiped & that ) : ResourceCgi(that) {}
	ResourcePiped & operator = (const ResourcePiped & ) { return (*this); }
public:
	ResourcePiped(void) : ResourceCgi(),
		pid(0), 
		ip(NULL), 
		op(NULL),
		stat(-1),
		xit(-1), 
		sig(-1)
	{}
	~ResourcePiped();
	
	int			init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn);
	
	void        push_body(void);
	int			status(void);
	void		conn_closed(void);
	int			wait(int opt);

	int			rem(EpollClient *epc);

private:
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;

	int			stat;
	int			xit;
	int			sig;

};

#endif