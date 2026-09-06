/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/06 23:04:06 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ResourceCgi.hpp"
#include "Server.hpp"


int	ResourceCgi::get_req_body(void)
{
	Session &sess = conn->sess;
	Request &req  = sess.getRequest();

	if (body.size())
		return (1);

	if (!req.hasHeaders())
		return (REQ_WAIT_HEAD);

	if (!req.hasBody())
	{
		if (req.isComplete())
			return (REQ_COMPLETE);
		return (REQ_WAIT_BODY);
	}

	Stream *rbody = req.getBody();

    char buf[REQ_READ_SIZ];
    ssize_t err = rbody->readsome(buf, REQ_READ_SIZ);
	if (err <= 0)
		return (REQ_COMPLETE);
    body.append(buf, err);
	return (body.size());
}

std::string & ResourceCgi::get_resp(void)
{
	if (this->resp_head.size())
		return (this->resp_head);
	return (this->resp_body);
}

bool	ResourceCgi::resp_data(void)
{
	return ((bool) this->get_resp().size());
}

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));
}

static std::string hedval_str(std::string & str, const char *key)
{
	std::string	kstr = std::string(key);
	std::string	val("");

	std::string::const_iterator it = std::search(
		str.begin(), str.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == str.end())
        return (val);
    if (it != str.begin() && *(it-1) != '\n')
		return (val);
		
	size_t line_beg = it - str.begin();
    std::stringstream	line(str.substr(line_beg));
    line >> kstr >> val;
	std::transform(val.begin(), val.end(), val.begin(), ::tolower);
	
	size_t line_end = str.find('\n', line_beg);
	str.erase(line_beg, line_end - line_beg + 1);

    return (val);

}

int		ResourceCgi::recv_data(char *buf, int siz)
{
	if (!this->have_head)
	{
		this->resp_head.append(buf, siz);
		return (this->chk_rsp_hed());
	}
		
	this->resp_body.append(buf, siz);
	if (!this->wait_comp)
	{
		conn->mod_evt(EPOLLOUT);
	}
	else
	{
		if (this->resp_body.size() > 512000)
		{
			WSCOL(WSL_CYAN);
			WSLOG(LVL_TMP, TGT_CGI_HEAD, "wait: OFF");

			// start CHUNKED
			if (this->clen)
			{
				this->wait_comp = false;
				this->ka = false;
				this->make_head();
				conn->mod_evt(EPOLLOUT);
			}
			else
			{
				// too much data without knowing clen
				WSCOL(WSL_RED);
				WSLOG(LVL_TMP, TGT_CGI_HEAD, "wait: MAXXED");
				this->set_err(500);
				return (RSRC_RESP_ERR);
			}
		}
	}
	return (RSRC_RESP_BODY);

}

int		ResourceCgi::chk_rsp_hed(void)
{
	size_t	pos = this->resp_head.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);
		
	this->resp_body = this->resp_head.substr(pos + 4);
	this->resp_head.erase(pos + 4);
	
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "wait: ", this->wait_comp);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "(ka): ", this->ka);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "head: ", this->resp_head.size());
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "body: ", this->resp_body.size());
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "head:\n", resp_head);

	std::string stat_str = hedval_str(resp_head, "Status");
	std::string conn_str = hedval_str(resp_head, "Connection");
	std::string clen_str = hedval_str(resp_head, "Content-Length");

	WSLOG(LVL_DBG, TGT_CGI_HEAD, "stat: ", stat_str);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "conn: ", conn_str);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "clen: ", clen_str);

	this->stat = std::atoi(stat_str.c_str());
	this->clen = std::atoi(clen_str.c_str());

#if 0 // WITH_KEEPALIVE
	if (this->clen)
	{
		// assume (clen) bytes will be delivered by script
		this->make_head();
	}
	else
	{
		this->wait_comp = true;
	}
#else
	this->ka = false;
	this->wait_comp = true;
#endif
	this->have_head = true;
	return (RSRC_RESP_HEAD);
}

void	ResourceCgi::make_head(void)
{
// stat
// conn
// clen
	std::string hed_str;
	
	hed_str = std::string("Cache-Control: no-cache\r\n");
	resp_head.insert(0, hed_str);
	if (this->clen)
	{
		hed_str = std::string("Content-Length: ") + toString(this->clen) + "\r\n";
		resp_head.insert(0, hed_str);
	}
	
	if (this->ka)
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  : keep-alive");
		hed_str = std::string("Connection: Keep-Alive\r\n");
		resp_head.insert(0, hed_str);
	}
	else
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  : close");
		hed_str = std::string("Connection: close\r\n");
		resp_head.insert(0, hed_str);
	}

	if (this->stat)
		hed_str = std::string("HTTP/1.0 ") + toString(this->stat) + "\r\n";
	else
		hed_str = std::string("HTTP/1.0 200 OK\r\n");
	resp_head.insert(0, hed_str);

}

void	ResourceCgi::chk_rsp_len(void)
{
	if (!this->have_head)
		return;
		
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "RLEN : head\n", this->resp_head);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "RLEN : body", this->resp_body.size());
	
	if (this->clen)
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "have : content-length");
		if (this->resp_body.size() != this->clen)
		{
			WSCOL(WSL_PURPLE);
			WSLOG(LVL_DBG, TGT_CGI_HEAD, "fix  : content-length");
		}
		else
		{
			WSCOL(WSL_GREEN);
			WSLOG(LVL_DBG, TGT_CGI_HEAD, "good : content-length");
		}
	}

	this->clen = this->resp_body.size();
	this->make_head();
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "DONE");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "head:\n", this->resp_head);
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "body:\n", this->resp_body);
}

int	ResourceCgi::set_err(int e)
{
	this->error = e;
	if (this->conn)
		return (this->conn->set_err(e));
	return (-1);
}

int	ResourceCgi::set_done(int d)
{
	this->done |= d;
	if (this->done & RSRC_DONE_ERR)
		return (-1);
	if (this->done & RSRC_DONE_OP)
		conn->mod_evt(EPOLLOUT);
	if ((this->done & RSRC_DONE_IO) == RSRC_DONE_IO)
		return (-1);
	return (0);
}
