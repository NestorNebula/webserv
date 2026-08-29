/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/29 11:49:13 by kdonlon          ###   ########.fr       */
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

	Stream * rbody = req.getBody();

    char buf[REQ_READ_SIZ];
    ssize_t err = rbody->readsome(buf, REQ_READ_SIZ);
    // WSLOG(LVL_DBG, TGT_CGI, "SOME: ", err);
	if (err <= 0)
		return (REQ_COMPLETE);
    body.append(buf, err);
    // WSLOG(LVL_DBG, TGT_CGI, "body: ", body.size());
	return (body.size());
}

int		ResourceCgi::recv_data(char *buf, int siz)
{
	this->resp.append(buf, siz);
	// ah -- continually flushing
	// good place to "see" WAIT_COMPLETE
	// WSLOG(LVL_DBG, TGT_RSRC, "resp: ", resp.size());

	WSLOG(LVL_NONE, TGT_RSRC, "resp");
	WSLOG(LVL_NONE, TGT_RSRC, "****\n", resp);
	
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
int		ResourceCgi::chk_rsp_hed(void)
{
	if (this->hed)
	{
#if !RES_CGI_WAIT_COMPLETE
		conn->mod_evt(EPOLLOUT);
#endif
		return (RSRC_RESP_BODY);
	}	
	
	size_t	pos = resp.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);
	// std::string hed = resp.substr(0, pos + 4);
		
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "resp:\n", resp);	
	this->hed = 1;
	
// REQUIRE (?) Content-Type (?)
	std::string conn_str = hedval_str(resp, "Connection");
	std::string clen_str = hedval_str(resp, "Content-Length");

	
// KEEP_ALIVE -- attention must be paid
	// if conn == keep-alive 
	// and we do NOT have content-length
	// we have a problem
	// if conn == close
	// set this->ka to FALSE -- no matter what
	// if conn empty
	// add keep-live IF we have content-length AND this->ka

#if !RES_CGI_WAIT_COMPLETE // -- chk_rsp_hed
	// how did FCGI overcome this (?) ignored by siege (?)
	std::string conn_close("Connection: close\r\n");
	resp.insert(0, conn_close);
#endif



	std::string stat_hed;
	std::string stat_str = hedval_str(resp, "Status");
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
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "RESP:\n", this->resp);	
	return (RSRC_RESP_HEAD);
}

// only makes sense for WAIT_COMPLETE
void	ResourceCgi::chk_rsp_len(void)
{

	// wow .. what if we started FLUSHING (?)
	// ASSUMES : we have not flushed
	// WAIT_COMPLETE
	// which -- I wanted DYNAMIC
	size_t	pos = resp.find("\r\n\r\n");
	// std::string hed = resp.substr(0, pos + 4);
	
	std::string kastr = std::string("\r\nConnection: Keep-Alive");

	
// chk_rsp_hed should have handled this check

	std::string clen_str = hedval_str(resp, "Content-Length");
	if (clen_str.size())
	{
		// we should have this from rsp_Hed
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CGI, "clen: ", clen_str);

// KEEP_ALIVE
// attn : do not over-ride (Connection: close)
		if (this->ka)
			resp.insert(pos, kastr);
		return;
	}

	
// if WAIT_COMPLETE 
	// we know content length
	size_t clen = (resp.size() - pos - 4);

	clen_str = std::string("\r\nContent-Length:") + toString(clen);
		// consider adding keep-alive
	WSCOL(WSL_GREEN);
	WSLOG(LVL_DBG, TGT_CGI, "clen: ", clen_str);
	
	resp.insert(pos, clen_str);
// KEEP_ALIVE
// attn : do not over-ride (Connection: close)
	if (this->ka)
		resp.insert(pos, kastr);
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
	if ((this->done & RSRC_DONE_IO) == RSRC_DONE_IO)
		return (-1);
	return (0);
}
