/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:48 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/01 17:34:11 by kdonlon          ###   ########.fr       */
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
# include "ServerConfig.hpp"

# ifndef EPOLL_MAX_EVT
#  define EPOLL_MAX_EVT 1024
# endif

# ifndef EPOLL_TIMEOUT
#  define EPOLL_TIMEOUT 1
# endif

std::string evt_type(int evt);

class EpollClient;

class Epoll
{
private:
	Epoll 			   (const Epoll & that) : 
		envp(that.envp) 
		{}
	Epoll & operator = (const Epoll & ) 
		{ return (*this); }

public:
	Epoll (char ** & _envp);
	~Epoll();

	int		loop(void);
	
	int		add(EpollClient *cli);
	int		mod(EpollClient *cli);
	int		del(EpollClient *cli);
	int		rem(EpollClient *cli);
	
	void	cleanup(void);
	
	int		serve(const std::vector<ServerConfig> &serv_list);

	size_t	num_cli(void) { return (this->clients.size()); }
	void	cli_info(void);
	
private:
	int						epfd;
	int						ecnt;
	struct epoll_event		evts[EPOLL_MAX_EVT];
	static const int		toms = (EPOLL_TIMEOUT * 900);
	
	std::set<EpollClient*>	clients;

	char 					**&envp;
	
	int						exec(void);
	struct epoll_event		*get_evt(int idx);
	EpollClient				*get_epc(void *cli);
	bool					has_client(EpollClient *ecp);

	void					check_timeo(void);
};

#endif

