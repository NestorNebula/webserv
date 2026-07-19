/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:21:06 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/19 13:48:31 by kdonlon          ###   ########.fr       */
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


// CONFIG
# ifndef EPC_TIMEOUT
#  define EPC_TIMEOUT 55
# endif

# ifndef EPC_BUF_SIZ
#  define EPC_BUF_SIZ 8192
# endif

// ATTN : small values .. 
// may not flush PIPE quickly enough
# ifndef EPC_OUT_SIZ
#  define EPC_OUT_SIZ 8192 // (3) -- good test for cgi-out
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
	EpollClient				 (const EpollClient & that) : 
		ep(that.ep), typ(that.typ), fd(that.fd) {}
		
	EpollClient (Epoll *_ep, epc_typ _typ, int _fd);
	
	virtual ~EpollClient();

	ssize_t			recv(void);
	ssize_t			send(const char *buf, ssize_t siz);
	ssize_t			send(std::string & str);
	ssize_t			send(std::string & str, ssize_t cnt);
	
	virtual ssize_t	pollin (void)   = 0;
	virtual ssize_t pollout(void)   = 0;
	virtual int		hup    (void)   = 0;
	virtual bool	timeo  (time_t) = 0;

	int				ini_evt(int e);
	int				mod_evt(int e);
	int				event(struct epoll_event *e);

	int					get_fd  (void) const;
	struct epoll_event	*get_evt(void);
    std::string 		typ_str(void);
	
protected:
	Epoll				*ep;
	epc_typ				typ;
	int					fd;
	struct epoll_event	evt;
	time_t				lact;
	int					error;
    char            	ibuf[EPC_BUF_SIZ];
};

#endif