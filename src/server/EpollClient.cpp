/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 00:44:51 by kdonlon          ###   ########.fr       */
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
	if (e < 0)
	{
		evt.events &= ~(-e);
	}
	else
	{
		evt.events |= e;
	}
	evt.events |= EPOLLRDHUP;
	return (this->ep->mod(this));
}

ssize_t	EpollClient::recv(void)
{
	ssize_t	err = 0;

	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "recv: ", err);
	
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_RECV, "read");
	if (err == 0)
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: zero"); // WARN
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
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: zero"); // WARN
	return (err);
}

ssize_t	EpollClient::send(std::string & str)
{
	ssize_t	err;

	err = this->send(str.c_str(), str.size());
	if (err <= 0)
		return (err);
	str.erase(0, err);
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
	epc_typ t;
	try
	{
		t = this->get_typ();
		if (t >= EPC_MAX)
			return (std::string(""));
		// WsLog::_(LVL_ERR, TGT_EPC, "typ", t);
		return (epc_str[t]);

	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (std::string(""));
}

void	EpollClient::set_lact(void)
{
	this->lact = time(&this->lact);
}
