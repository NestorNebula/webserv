/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:21:06 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 11:48:38 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_CLIENT_HPP
# define EPOLL_CLIENT_HPP

# include <unistd.h>
# include <sys/epoll.h>
# include <fcntl.h>
# include <string>
# include <vector>
# include "WsLog.hpp"
# include "WsTime.hpp"
# include "SizeDefs.hpp"


typedef enum
{
	EPC_SERV,
	EPC_CONN,
	EPC_CGI,
	EPC_FCGI,
	EPC_MAX
}	epc_typ;
// ATTN : typ_str

class Epoll;

class EpollClient
{
protected:
    char	ibuf[EPC_BUF_SIZ];
	
private:
	EpollClient & operator = (const EpollClient & ) 
		{ return (*this); }
public:
	EpollClient				 (const EpollClient & that) : 
		ep(that.ep), typ(that.typ), fd(that.fd) {}
		
	EpollClient (Epoll *_ep, epc_typ _typ, int _fd);
	
	virtual ~EpollClient();

	ssize_t			recv(void);
	ssize_t			send(const char *buf, ssize_t siz);
	ssize_t			send(std::string & str);
	ssize_t			send(std::string & str, ssize_t cnt);
	
	virtual ssize_t	pollin (void)	  = 0;
	virtual ssize_t pollout(void)	  = 0;
	virtual int		rdhup  (void)	  = 0;
	virtual int		hup    (void)	  = 0;
	virtual bool	timeo  (WsTime &) = 0;

	int					ini_evt(int e);
	int					mod_evt(int e);
	int					event(struct epoll_event *e);

	int					get_fd  (void) const;
	struct epoll_event	*get_evt(void);
	epc_typ				get_typ(void);
    std::string 		typ_str (void);
	
	Epoll				*ep;
protected:
	epc_typ				typ;
	int					fd;
	struct epoll_event	evt;
	WsTime				lact;
	int					error;
};

#endif