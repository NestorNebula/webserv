/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 11:45:13 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>
# include "Epoll.hpp"
# include "EpollClient.hpp"
# include <iostream>
# include <sys/wait.h>
# include <cstring>

class Server;

class Connection : public EpollClient
{
private:
	Connection (const Connection & that);
	Connection & operator = (const Connection & that) { return (*this); }
public:
	Server		&serv;
	
	Connection (int _fd, Server &_serv);
	~Connection();
	
	int			pollin(void);
	int			pollout(void);
	
private:
	int			exec_cgi(void);
	
	int			req_cnt;
	int			filedes; // "response"
	
};

#endif

