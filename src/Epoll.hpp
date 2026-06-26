/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:48 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/20 23:46:01 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_HPP
# define EPOLL_HPP

# include <iostream>
# include <unistd.h>
# include <sys/epoll.h>


# ifndef EPOLL_MAX_EVT
#  define EPOLL_MAX_EVT 128
# endif


class EpollClient
{
public:
	virtual ~EpollClient() {} ;
	virtual int	get_fd(void) = 0;
	virtual int	pollin(void) = 0;
	virtual int pollout(void) = 0;
};

class Epoll
{
public:
	Epoll (void);
	Epoll (const Epoll & that);
	Epoll & operator = (const Epoll & that);
	~Epoll();

	int		exec(void);
	
	int		add(int fd, int e, void *data);
	int		mod(int fd, int e, void *data);
	int		del(int fd);
	
	struct epoll_event	*get_evt(int idx);
	
private:
	int					epfd;
	int					ecnt;
	struct epoll_event	evts[EPOLL_MAX_EVT];
};

std::ostream& operator << (std::ostream & os, Epoll & obj);

void	evt_typ(struct epoll_event *evt);

#endif

