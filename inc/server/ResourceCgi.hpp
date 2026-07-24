/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 17:52:57 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "Connection.hpp"
# include "CgiPipe.hpp"
# include "bridge.hpp"


class ResourceCgi : public Resource
{
private:
	ResourceCgi				 (const ResourceCgi & ) {}
	ResourceCgi & operator = (const ResourceCgi & ) { return (*this); }
public:
	ResourceCgi(void) : pid(0), ip(NULL), op(NULL), stat(-1), hed(0), clen(0), hlen(0), tlen(0), slen(0), xit(-1), sig(-1), ka(0), error(0) {}
	~ResourceCgi();

	int			init(Epoll *ep, pid_t _pid, cgi_pipes *pipes, Connection *conn);
	
	void        push_data(void);
	void		set_err(int e) { this->error = e; }
	
	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
	int			stat;
	int			hed;
	int			clen;
	int			hlen;
	int			tlen;
	int			slen;
	int			xit;
	int			sig;
	int			ka;
	int			error;

	int			status(int opt);
	int			rem(CgiPipe *epc);
	void		reset(void);
};


#endif