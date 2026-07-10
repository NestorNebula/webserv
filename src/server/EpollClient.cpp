/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 12:58:37 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollClient.hpp"
#include "Epoll.hpp"

// EP
EpollClient::EpollClient::EpollClient(Epoll *_ep, epc_typ _typ, int _fd) : 
	ep(_ep),
	typ(_typ), 
	fd(_fd), 
	lact(0), 
	error(0)
{
	evt.events = 0;
	evt.data.ptr = NULL;
}

EpollClient::~EpollClient()
{
	WsLog::_(LVL_DBG, TGT_EPC, "(~) EpollClient");
	if (this->fd != -1)
		close(this->fd);
}

int	EpollClient::ini_evt(int e)
{
	if (evt.data.ptr != NULL)
	{
		WsLog::_(LVL_WARN, TGT_EPC, "ini_evt: already initialized");
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
		WsLog::_(LVL_WARN, TGT_EPC, "mod_evt: not yet initialized");
		return (this->ini_evt(e));
	}
	evt.events = e;
	evt.events |= EPOLLRDHUP;
	return (this->ep->mod(this));
}

int	EpollClient::event(struct epoll_event *e)
{
	int err;

	this->lact = time(&this->lact);
	if (e->events & EPOLLERR)
		return (-1);
	if (e->events & EPOLLIN)
	{
		err = this->pollin();
		if (err < 0) // && state (?)
			return (err);
	}
	if (e->events & EPOLLOUT)
	{
		err = this->pollout();
		if (err < 0) // && state (?)
			return (err);
	}	
		// nothing more to read ...
		// ... may still need to write (cgi) output (?)
	if (e->events & EPOLLRDHUP)
	{
		// this->hup();
		// epc->mod_evt(EPOLLOUT);
		return (-1);
	} 
		// cgi : may close with (hup)
	if (e->events == EPOLLHUP)
	{
		this->hup();
		return (-1);
	}
	return (0);	
}

ssize_t	EpollClient::recv(void)
{
	ssize_t	err = 0;

	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_DBG, TGT_EPC_RECV, "recv: ", err);
	
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_RECV, "read");
	if (err == 0)
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: zero");
	return (err);
}

ssize_t	EpollClient::send(const char *buf, size_t siz)
{
	ssize_t err;
	
	WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: ", siz);

	if (siz > EPC_OUT_SIZ)
		siz = EPC_OUT_SIZ;

	err = write(this->fd, buf, siz);

	WsLog::_(LVL_DBG, TGT_EPC_SEND, "sent: ", err);
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_SEND, "write");
	if (err == 0)
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: zero");
	return (err);
}

ssize_t	EpollClient::send(std::string & str)
{
	ssize_t	err;

	err = this->send(str.c_str(), str.size());
	if (err <= 0)
		return (err);
	str.erase(0, err); // here (?) or caller (?)
	return (err);
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
