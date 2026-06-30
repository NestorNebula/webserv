/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:48 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 15:29:43 by kdonlon          ###   ########.fr       */
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

typedef enum
{
	EPC_SERV,
	EPC_CONN,
	EPC_CGI
}	epc_typ;

typedef enum
{
	EPC_STATE_INIT,
	EPC_STATE_SHUTDOWN,
	EPC_STATE_ERROR,
	EPC_STATE_MAX
}	epc_state;

class EpollClient
{
public:
	EpollClient(epc_typ _typ, int _fd) : typ(_typ), fd(_fd), state(EPC_STATE_INIT), error(0) {}
	
	virtual ~EpollClient()
	{
		WsLog::_(LVL_ERR, TGT_EPC, "delete");
		if (this->fd != -1)
			close(this->fd);
	}
	virtual int	pollin(void) = 0;
	virtual int pollout(void) = 0;

	int			recv(void);
	int			send(std::string & buf);
	
	int			get_fd(void)	const { return (this->fd); }
	epc_typ		get_typ(void)	const { return (this->typ); }
	epc_state	get_state(void)	const { return (this->state); }
protected:
	epc_typ		typ;
	int			fd;
	epc_state	state;
	int			error;

public:
	std::string		ibuf;
	std::string		obuf;
};

const char *epc_type(EpollClient *epc); // silly : internal


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
	
	void	copied(void);
	
private:
	int					epfd;
	int					ecnt;
	struct epoll_event	evts[EPOLL_MAX_EVT];
	
	std::set<EpollClient*>	conn;

	int					exec(void);
	struct epoll_event	*get_evt(int idx);
};

std::ostream& operator << (std::ostream & os, Epoll & obj);

#endif

