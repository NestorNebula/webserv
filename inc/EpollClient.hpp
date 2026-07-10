/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:21:06 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 12:57:55 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_CLIENT_HPP
# define EPOLL_CLIENT_HPP

# include <unistd.h>
# include <sys/epoll.h>
# include <string>
# include <vector>
# include "WsLog.hpp"


# ifndef EPC_BUF_SIZ
#  define EPC_BUF_SIZ 4096
# endif

# ifndef EPC_OUT_SIZ
#  define EPC_OUT_SIZ 4096 // (3) -- good test for cgi-out
# endif

typedef enum
{
	EPC_SERV,
	EPC_CONN,
	EPC_CGI,
	EPC_MAX
}	epc_typ;
// ATTN : typ_str

class Epoll;

class EpollClient
{
private:
	EpollClient & operator = (const EpollClient & ) 
		{ return (*this); }
public:
	EpollClient (Epoll *_ep, epc_typ _typ, int _fd);
	EpollClient (const EpollClient & that) : 
		ep(that.ep), typ(that.typ), fd(that.fd) {}
	virtual ~EpollClient();

	ssize_t			recv(void);
	ssize_t			send(const char *buf, size_t siz);
	ssize_t			send(std::string & str);
	
	virtual ssize_t	pollin(void) = 0;
	virtual ssize_t pollout(void) = 0;
	virtual int		hup(void) = 0;
	virtual bool	timeo(time_t now) = 0;

	int				ini_evt(int e);
	int				mod_evt(int e);
	int				event(struct epoll_event *e);

	int					get_fd(void) const
		{ return (this->fd); }
	struct epoll_event	*get_evt(void)
		{ return (&this->evt); }
		
    std::string 		typ_str(void);
protected:
	Epoll				*ep;
	epc_typ				typ;
	int					fd;
	struct epoll_event	evt;
	time_t				lact;

public:
	int					error;
	
public:
    char            	ibuf[EPC_BUF_SIZ];
	std::string			istr;
	std::string			ostr;
};

#endif