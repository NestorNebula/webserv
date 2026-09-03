/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourcePiped.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:15:32 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 21:30:52 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_PIPED_HPP
# define RESOURCE_PIPED_HPP

# include "ResourceCgi.hpp"
# include "CgiPipe.hpp"

class ResourcePiped : public ResourceCgi
{
private:
	ResourcePiped			   (const ResourcePiped & that ) : ResourceCgi(that) {}
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
	void		conn_closed(void);
	int			status(void);
	int			wait(int opt);
	int			rem(EpollClient *epc);

	pid_t		pid;
	CgiPipe		*ip;
	CgiPipe		*op;
private:

	int			stat;
	int			xit;
	int			sig;
	void		set_failed(void);
};

#endif