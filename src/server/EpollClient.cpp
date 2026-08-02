/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/02 19:07:11 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollClient.hpp"
#include "Epoll.hpp"

EpollClient::EpollClient::EpollClient(Epoll *_ep, epc_typ _typ, int _fd) : 
	ep(_ep),
	typ(_typ), 
	fd(_fd), 
	lact(0),
	error(0),
	REM(0)
{
	evt.events = 0;
	evt.data.ptr = NULL;
}

EpollClient::~EpollClient()
{
	WsLog::_(LVL_DBG, TGT_EPC, " (~) EpollClient");
	if (this->fd != -1)
		close(this->fd);
}

int	EpollClient::ini_evt(int e)
{
	if (evt.data.ptr != NULL)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "ini_evt  : already initialized");
		return (this->mod_evt(e));
	}
	evt.data.ptr = this;
	evt.events = e;
	evt.events |= EPOLLRDHUP;
	return (this->ep->add(this));
}

int	EpollClient::mod_evt(int e)
{
	if (evt.data.ptr == NULL)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "mod_evt  : not yet initialized");
		return (this->ini_evt(e));
	}

	// WsLog::_(LVL_WARN, TGT_EPC, "mod  : ", e);
	// WsLog::_(LVL_WARN, TGT_EPC, "cur  : ", (evt.events & ~(EPOLLRDHUP)));
	// if (e == (int) (evt.events & ~(EPOLLRDHUP)))
	// 	return (0);
	
	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod_evt  : CUR ", evt_type(evt.events));
	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod_evt  : MOD ", evt_type(e));
	
	if (e == 0)
		evt.events = e;
	else if (e < 0)
	{
		e = -e;
		evt.events &= ~e;
	}
	else
	{
		if (evt.events == (e | EPOLLRDHUP))
		{
			WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod_evt  : no change");
			return (0);
		}
		evt.events |= e;
	}
	
	evt.events |= EPOLLRDHUP;
	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod_evt  : RES ", evt_type(evt.events));

	return (this->ep->mod(this));
}

int	EpollClient::event(struct epoll_event *e)
{
	int err;

	this->lact = time(&this->lact);
	if (e->events & EPOLLERR)
	{
		this->hup();
		return (-1);
	}
	if (e->events & EPOLLIN)
	{
		// if (fcntl(this->fd, F_GETFD) < 0)
		// 	WsLog::_(LVL_ERR, TGT_EPC, "evt : BAD FD");
			
		err = this->pollin();
		if (err < 0)
			return (err);
	}
	if (e->events & EPOLLOUT)
	{
		// if (fcntl(this->fd, F_GETFD) < 0)
		// 	WsLog::_(LVL_ERR, TGT_EPC, "evt : BAD FD");
		err = this->pollout();
		if (err < 0)
			return (err);
	}	
	if (e->events & EPOLLRDHUP)
		return (this->rdhup());
	if (e->events == EPOLLHUP)
		return (this->hup());
	return (0);	
}

ssize_t	EpollClient::recv(void)
{
	ssize_t	err = 0;

	// if (fcntl(this->fd, F_GETFD) < 0)
	// 	return (-1);
	
	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_DBG, TGT_EPC_RECV, "read: ", err);
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_RECV, "read");
	if (err == 0)
		WsLog::_(LVL_DBG, TGT_EPC_RECV, "read:  ZERO");
	return (err);
}

ssize_t	EpollClient::send(const char *buf, ssize_t siz)
{
	ssize_t err;
	
	// if (fcntl(this->fd, F_GETFD) < 0)
	// 	return (-1);
	
	WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: ", siz);

	if (siz > EPC_OUT_SIZ)
		siz = EPC_OUT_SIZ;

	err = write(this->fd, buf, siz);

	WsLog::_(LVL_DBG, TGT_EPC_SEND, "sent: ", err);
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_SEND, "write");
	if (err == 0)
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send:  ZERO");
	return (err);
}

ssize_t	EpollClient::send(std::string & str)
{
	if (str.size() == 0)
		return (0);
		
	ssize_t	err;

	err = this->send(str.c_str(), str.size());
	if (err <= 0)
		return (err);
	str.erase(0, err);
	return (err);
}

ssize_t	EpollClient::send(std::string & str, ssize_t cnt)
{
	ssize_t	err;

	err = this->send(str.c_str(), cnt);
	if (err <= 0)
		return (err);
	str.erase(0, err);
	return (err);
}


int	EpollClient::get_fd  (void) const
{
	return (this->fd);
}

void	EpollClient::set_rem (int r)
{
	this->REM = r;
}

int	EpollClient::get_rem (void) const
{
	return (this->REM);
}

struct epoll_event	*EpollClient::get_evt(void)
{
	return (&this->evt);
}
	
static const char *epc_str[] = 
{
	"serv",
	"conn",
	"cgi",
	NULL
};

std::string EpollClient::typ_str(void)
{
	return (epc_str[this->typ]);
}
