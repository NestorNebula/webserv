/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:48 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/03 09:37:28 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_HPP
# define EPOLL_HPP

# include <iostream>
# include <unistd.h>
# include <sys/epoll.h>
# include <signal.h>
# include <cerrno>
# include <set>

# include "WsLog.hpp"

# ifndef EPOLL_MAX_EVT
#  define EPOLL_MAX_EVT 128
# endif

class EpollClient;

class Epoll
{
private:
	Epoll (const Epoll & that) { (void) that;}
	Epoll & operator = (const Epoll & that) { (void) that; return (*this); }

public:
	Epoll (void);
	~Epoll();

	int		loop(void);
	
	int		add(EpollClient *cli, int e);
	int		mod(EpollClient *cli, int e);
	int		del(EpollClient *cli);
	int		rem(EpollClient *cli);
	
private:
	int					epfd;
	int					ecnt;
	struct epoll_event	evts[EPOLL_MAX_EVT];
	
	std::set<EpollClient*>	clients;

	int					exec(void);
	struct epoll_event	*get_evt(int idx);
	EpollClient			*get_epc(void *cli);
	bool				has_client(EpollClient *ecp);
};

#endif

