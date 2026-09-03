/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 21:21:59 by kdonlon          ###   ########.fr       */
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
	return (this->resp);
}

bool	ResourceCgi::resp_data(void)
{
	if (this->resp.size())
		return (true);
	return (false);

}

int		ResourceCgi::recv_data(char *buf, int siz)
{
	this->resp.append(buf, siz);
	
	WSLOG(LVL_NONE, TGT_RSRC, "resp");
	WSLOG(LVL_NONE, TGT_RSRC, "****\n", resp);

	if (this->hed)
	{
		if (!this->wait_comp)
			conn->mod_evt(EPOLLOUT);
		return (RSRC_RESP_BODY);
	}
	return (this->chk_rsp_hed());
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

    std::stringstream	line(str.substr(it - str.begin()));
    line >> kstr >> val;

	std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    return (val);

}


// WAIT_COMP WORK HERE 
// called by : recv_data
// CLARIFY : the rules for
	// keep-alive 

// parse_rsp_hed ... 
// build .. from passed headers


// cooler : separate head/body .. HEAD request
// we could then re-construct head
// replacing Connection:

// this is where we should SET WAIT_COMPLETE

// CONSIDER : CHUNKED (!) .. if 
// no content-length
// OR 
// content->length TOO LONG


int		ResourceCgi::chk_rsp_hed(void)
{
	size_t	pos = resp.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);

	this->hed = 1;
	
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "resp:\n", resp.substr(0, pos));

	std::string stat_str = hedval_str(resp, "Status");
	std::string conn_str = hedval_str(resp, "Connection");
	std::string clen_str = hedval_str(resp, "Content-Length");

	WSLOG(LVL_DBG, TGT_CGI_HEAD, "stat: ", stat_str);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "conn: ", conn_str);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "clen: ", clen_str);

// because .. true by default (?)
	// if (clen_str.size())
	// 	this->wait_comp = false;
		
// KEEP_ALIVE : rsp_hed .. add if CGI returned (content-length)
	if (false) // clen_str.size() && this->ka)
	{
		// and : connection not already set -- 
		// which a CGI script should never do .. 
		WSCOL(WSL_GREEN);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  keep-alive");
		std::string kastr = std::string("Connection: Keep-Alive\r\n");
		resp.insert(0, kastr);
	}
	else if (true) // !this->wait_comp)
	{
		// WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  close");
		std::string conn_close("Connection: close\r\n");
		resp.insert(0, conn_close);
// KEEP_ALIVE : NOT wait_comp - force connection : close
		this->ka = false;
	}

	std::string stat_hed;
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_str);
	if (stat_str.size())
	{
		// HTTP/1.1 STATUS [Status Message]
// WEBSERV : this->set_err()
		stat_hed = std::string("HTTP/1.0 ") + stat_str + "\r\n";
	}
	else
	{
		stat_hed = std::string("HTTP/1.0 200 OK\r\n");
	}
	resp.insert(0, stat_hed);

	// pos = resp.find("\r\n\r\n");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "RESP:\n", resp.substr(0, pos));
	return (RSRC_RESP_HEAD);
}



// very important part of wait_comp
// only called if wait_comp
// which we only set when ... 
// so .. we assume .. no content-length was provided
void	ResourceCgi::chk_rsp_len(void)
{
	if (this->hed == 0)
		return;
		
	size_t	pos = resp.find("\r\n\r\n");

	WSLOG(LVL_DBG, TGT_CGI_HEAD, "RLEN\n", resp.substr(0, pos));
	
	std::string clen_str = hedval_str(resp, "Content-Length");
	if (clen_str.size())
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "have content-length\n", clen_str);
		return;
	}

// insert  REAL CLEN 
	size_t clen = (resp.size() - pos - 4);

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "clen: resp ", clen);

	clen_str = std::string("\r\nContent-Length:") + toString(clen);
	
	WSCOL(WSL_GREEN);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  content-length ", clen_str);

	resp.insert(pos, clen_str);


// keep-alive : what was (already?) set in chk_rsp_hed
// KEEP_ALIVE - have complete response, add keep-alive header
// KEEP_ALIVE - assumes wait_comp (?)
	if (this->ka)
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  keep-alive");
		std::string kastr = std::string("\r\nConnection: Keep-Alive");
		resp.insert(pos, kastr);
	}
	else
	{
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  close");
		std::string conn_close("\r\nConnection: close");
		resp.insert(pos, conn_close);
	}
	pos = resp.find("\r\n\r\n");
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "DONE");
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "resp:\n", resp.substr(0, pos));
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
