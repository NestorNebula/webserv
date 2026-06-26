/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:48 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 19:03:46 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_HPP
# define EPOLL_HPP

# include <iostream>
# include <unistd.h>
# include <sys/epoll.h>
# include <signal.h>
# include <set>

# ifndef EPOLL_MAX_EVT
#  define EPOLL_MAX_EVT 128
# endif

# ifndef DBG_EPOLL
#  define DBG_EPOLL 0
# endif

typedef enum
{
	EPC_SERV,
	EPC_CONN
}	epc_typ;

class EpollClient
{
public:
	EpollClient(epc_typ _typ, int _fd) : typ(_typ), fd(_fd) {}
	
	virtual ~EpollClient() {} ;
	virtual int	pollin(void) = 0;
	virtual int pollout(void) = 0;
	
	int		get_fd(void) const { return (this->fd); }
	epc_typ	get_typ(void) const { return (this->typ); }
protected:
	epc_typ	typ;
	int		fd;
};



class Epoll
{
public:
	Epoll (void);
	Epoll (const Epoll & that);
	Epoll & operator = (const Epoll & that);
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
	
	std::set<EpollClient*>	conn;

	int					exec(void);
	struct epoll_event	*get_evt(int idx);
};

std::ostream& operator << (std::ostream & os, Epoll & obj);

void	evt_typ(struct epoll_event *evt);

#endif

