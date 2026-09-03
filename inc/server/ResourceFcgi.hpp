/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceFcgi.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/21 00:11:49 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 21:28:09 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_FCGI_HPP
# define RESOURCE_FCGI_HPP

# include "ResourceCgi.hpp"
# include "FcgiPipe.hpp"

class ResourceFcgi : public ResourceCgi
{
private:
	ResourceFcgi			  (const ResourceFcgi & that ) : ResourceCgi(that) {}
	ResourceFcgi & operator = (const ResourceFcgi & ) { return (*this); }
public:
	ResourceFcgi(void) : ResourceCgi(), fcgi(NULL) {}
	~ResourceFcgi();

	int			init(Epoll *ep, CgiEnv *cgienv, Connection *conn, std::string &sock_path);

	void        push_body(void);
	void		conn_closed(void);
	int			status(void);
	int			wait(int opt);
	int			rem(EpollClient *epc);
private:
	FcgiPipe	*fcgi;
};

#endif