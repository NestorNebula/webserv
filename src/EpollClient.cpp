/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 23:12:29 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollClient.hpp"

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



#define EPC_TO_SECS 5

int	EpollClient::timeout(void)
{
	int	err = 0;

	time_t now = time(&now);
	if (this->lact)
	{
		double secs = ((double) (now - lact));
		if (secs > EPC_TO_SECS)
		{
			WsLog::_(LVL_WARN, TGT_CONN, "timed out");
			err = 1;
		}
	}
	this->lact = now;
	return (err);
}

int	EpollClient::recv(void)
{
	int	err = 0;

	// char	buf
	err = read(this->fd, this->ibuf, EPC_BUF_SIZ);
	
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "recv");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, err);
	
	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: ", strerror(errno));
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: zero");
		// this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}
	this->ibuf[err] = '\0';

	WsLog::_(LVL_INFO, TGT_EPC_RECV, "ibuf");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "\n", this->ibuf);

	if (err == EPC_BUF_SIZ)
	{
		// more to read
		// epoll should let us know 
	}
	
	return (err);
}

int	EpollClient::send(std::string & buf)
{
	int err;
	
	WsLog::_(LVL_INFO, TGT_EPC_SEND, "send");
	WsLog::_(LVL_INFO, TGT_EPC_SEND, buf.size());

	size_t osiz = EPC_OUT_SIZ;
	if (osiz > buf.size())
		osiz = buf.size();
	err = write(this->fd, buf.c_str(), osiz);

	WsLog::_(LVL_INFO, TGT_EPC_SEND, "sent");
	WsLog::_(LVL_INFO, TGT_EPC_SEND, err);

	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		WsLog::_(LVL_ERR, TGT_EPC_SEND, "send: ", strerror(errno));
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: zero");
		// this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}
	buf.erase(0, err);
	return (err);
}
