/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:23:28 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 22:31:43 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "EpollClient.hpp"

EpollClient::EpollClient::EpollClient(epc_typ _typ, int _fd) : 
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

int	EpollClient::recv(void)
{
	int	err = 0; // size_t

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

	WsLog::_(LVL_INFO, TGT_EPC_RECV, "ibuf");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "\n", this->ibuf);

	if (err == EPC_BUF_SIZ)
	{
		// more to read
		// epoll should let us know 
	}
	
	return (err);
}

// send(EpollBuf *)
// Conn : sends output from resource
	// file_read
	// cgi_exec
// Conn : 
	// rsrc.write_data()
		// file upload
		// cgi exec
		// may be more to write ..
		
	// rsrc.read_data() -- returns current EpollBuf of resources
// What problems does <stream> solve, and at what cost?

int	EpollClient::send(const char *buf, size_t siz)
{
	int err; // size_t
	
	WsLog::_(LVL_DBG, TGT_EPC_SEND, "send: ", siz);

	size_t osiz = EPC_OUT_SIZ;
	if (osiz > siz)
		osiz = siz;

	// while (osiz) ... 
	// osiz -= err;
	// NO : the promise here .. is that we only write fixed-max amount of data
	// per (fd) per call ...
	// seems inefficient, if only one socket is active
	// ensures : distribution is better
	err = write(this->fd, buf, osiz);

	WsLog::_(LVL_DBG, TGT_EPC_SEND, "sent: ", err);

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
