/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 16:49:16 by kdonlon          ###   ########.fr       */
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
	state(EPC_STATE_INIT), 
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

int	EpollClient::recv(void)
{
	int	err = 0; // size_t

	// EpollBuf::setup(EPC_BUF_SIZ)
		// malloc .. to receive enough
	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "recv: ", err);
	
	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		return WsLog::_errno(LVL_ERR, TGT_EPC_RECV, "read");
	}
	if (err == 0)
	{
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: zero");
		return (-1);
	}
	// "string" ASSUMED
	// ATTN : binary data
	// this->isiz = err;
	this->ibuf[err] = '\0';

	return (err);
}

int	EpollClient::send(const char *buf, size_t siz)
{
	int err; // size_t
	
	// EpollBuf::setup -- from where .. and how much .. to send
	WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: ", siz);

	size_t osiz = EPC_OUT_SIZ;
	if (osiz > siz)
		osiz = siz;

	err = write(this->fd, buf, osiz);

	WsLog::_(LVL_DBG, TGT_EPC_SEND, "sent: ", err);

// NEED : to track if all bytes not sent ... 
	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		return WsLog::_errno(LVL_ERR, TGT_EPC_SEND, "write");
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: zero");
		return (-1);
	}
	return (err); // bytes written
}

// get rid of this
int	EpollClient::send(std::string & buf)
{
	int	err; // size_t

	err = this->send(buf.c_str(), buf.size());
	if (err <= 0)
		return (err);
		// UGLY
	buf.erase(0, err); // hm : here (?) or in parent
	return (err); // bytes written
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
