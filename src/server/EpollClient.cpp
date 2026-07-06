/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 21:02:37 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollClient.hpp"
#include "Epoll.hpp"

// EP
EpollClient::EpollClient::EpollClient(Epoll & _ep, epc_typ _typ, int _fd) : 
	ep(_ep),
	typ(_typ), 
	fd(_fd), 
	lact(0), 
	error(0)
{
}

EpollClient::~EpollClient()
{
	// WsLog::_(LVL_ERR, TGT_EPC, "(~) EpollClient");
	if (this->fd != -1)
		close(this->fd);
}

int	EpollClient::mod_evt(int e)
{
	return (this->ep.mod(this, e));
}

ssize_t	EpollClient::recv(void)
{
	ssize_t	err = 0;

	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "recv: ", err);
	
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPC_RECV, "read");
	if (err == 0)
	{
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: zero");
		return (0);
	}
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
	{
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: zero");
		return (0);
	}
	return (err); // bytes
}

ssize_t	EpollClient::send(std::string & buf)
{
	ssize_t	err;

	err = this->send(buf.c_str(), buf.size());
	if (err <= 0)
		return (err);
	buf.erase(0, err);
	return (err); // bytes
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
