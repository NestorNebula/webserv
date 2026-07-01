/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:21:06 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/01 07:47:14 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_CLIENT_HPP
# define EPOLL_CLIENT_HPP

# include <unistd.h>
# include <string>
# include "WsLog.hpp"


# ifndef EPC_BUF_SIZ
#  define EPC_BUF_SIZ 4095
# endif

# ifndef EPC_OUT_SIZ
#  define EPC_OUT_SIZ 4095
# endif

typedef enum
{
	EPC_SERV,
	EPC_CONN,
	EPC_CGI,
	EPC_MAX
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
	EpollClient(epc_typ _typ, int _fd) : typ(_typ), fd(_fd), lact(0), state(EPC_STATE_INIT), error(0) {}
	
	virtual ~EpollClient()
	{
		// WsLog::_(LVL_ERR, TGT_EPC, "(~) EpollClient");
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

    std::string typ_str(void);
    
protected:
	epc_typ		typ;
	int			fd;
	time_t		lact;
	int			timeout(void);
public:
	epc_state	state;
	int			error;

public:
    char            ibuf[EPC_BUF_SIZ + 1];
	std::string		istr;
	std::string		ostr;
};

#endif