/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/30 16:08:58 by kdonlon          ###   ########.fr       */
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

std::string & ResourceCgi::get_resp(void)
{
	if (this->resp.size())
		return (this->resp);
	if (this->tfs)
	{
		// WSCOL(WSL_YELLOW);
		// WSLOG(LVL_TMP, TGT_RSRC, "tfs : read");
		this->tfs->read(this->resp);

		if (this->tfs->eof())
		{
			WSCOL(WSL_YELLOW);
			WSLOG(LVL_TMP, TGT_RSRC, "tfs : EOF");
			delete (this->tfs);
			this->tfs = NULL;
		}
	}

	return (this->resp);
}
bool	ResourceCgi::resp_data(void)
{
	if (this->resp.size())
		return (true);
	if (this->tfs && !this->tfs->eof())
		return (true);
	return (false);

}
int		ResourceCgi::recv_data(char *buf, int siz)
{
	if (this->tfs)
	{
		// WSCOL(WSL_YELLOW);
		// WSLOG(LVL_TMP, TGT_RSRC, "tfs : write");
		tfs->write(buf, siz);
	}
	else
	{
		this->resp.append(buf, siz);
	}

	WSLOG(LVL_NONE, TGT_RSRC, "resp");
	WSLOG(LVL_NONE, TGT_RSRC, "****\n", resp);

	if (this->hed)
	{
// #if !RES_CGI_WAIT_COMPLETE
		if (this->wait_comp)
		{
			// check resp size
			if (tfs == NULL && this->resp.size() > 2000000)
			{
				WSCOL(WSL_YELLOW);
				WSLOG(LVL_TMP, TGT_RSRC, "resp: temp file");
// When Nginx talks to a backend handler (like a FastCGI server), it saves the response data into internal memory buffers before sending it to the client. If the response is larger than the assigned buffer space, Nginx writes the extra data into a temporary file on the disk.

// so .. even large content-length
// would want a temp file 
// always buffer : 
// BUT : 
// write-to-file IF 
	// content-length > MAX
	// OR
	// content-length NOT DEFINED
				try
				{
					this->tfs = new TemporaryFileStream;
				}
				catch(const std::exception& e)
				{
					std::cerr << e.what() << '\n';
				
					WSLOG(LVL_DBG, TGT_RSRC, "wait_comp: too big");
					this->set_err(503); // CGI_ERR : file_size
					return (RSRC_RESP_ERR);

					// OR : kill wait_comp
					// 
				}
			}
		}
		else
		{
			conn->mod_evt(EPOLLOUT);
		}
// #endif
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

// called by : recv_data
int		ResourceCgi::chk_rsp_hed(void)
{


// this is where we MIGHT force wait complete
// if we do not have content-length


	size_t	pos = resp.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (RSRC_RESP_INIT);

	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "resp:\n", resp.substr(0, pos));
	this->hed = 1;

// REQUIRE (?) Content-Type (?)

// cooler : separate head/body .. HEAD request
// we could then re-construct head
// replacing Connection:

// this is where we should SET WAIT_COMPLETE


	std::string conn_str = hedval_str(resp, "Connection");
	std::string clen_str = hedval_str(resp, "Content-Length");

	// if (clen_str.size())
	// 	this->wait_comp = false;
		
	if (clen_str.size() && this->ka)
	{
		WSCOL(WSL_GREEN);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  keep-alive");
		std::string kastr = std::string("Connection: Keep-Alive\r\n");
		resp.insert(0, kastr);
	}
	else if (!this->wait_comp)
	{
// #if !RES_CGI_WAIT_COMPLETE
		WSCOL(WSL_RED);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  close");
		std::string conn_close("Connection: close\r\n");
		resp.insert(0, conn_close);
		this->ka = false;
// #endif
	}

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

	// pos = resp.find("\r\n\r\n");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "RESP:\n", resp.substr(0, pos));
	return (RSRC_RESP_HEAD);
}

// RES_CGI_WAIT_COMPLETE
void	ResourceCgi::chk_rsp_len(void)
{
	if (this->hed == 0)
		return;
	size_t	pos = resp.find("\r\n\r\n");

	WSLOG(LVL_DBG, TGT_CGI_HEAD, "RLEN\n", resp.substr(0, pos));
	
	// should already know this stuff
	std::string clen_str = hedval_str(resp, "Content-Length");
	if (clen_str.size())
	{
		WSCOL(WSL_YELLOW);
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "have content-length\n", clen_str);
		return;
	}

	size_t clen = (resp.size() - pos - 4);

	WSCOL(WSL_YELLOW);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "clen: resp ", clen);
	if (this->tfs)
	{
		WSLOG(LVL_DBG, TGT_CGI_HEAD, "clen: tfs  ", this->tfs->size());
		clen += this->tfs->size();
		// right value here ..
		// two extra bytes sent
	}

	clen_str = std::string("\r\nContent-Length:") + toString(clen);
	
	WSCOL(WSL_GREEN);
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "add  content-length ", clen_str);

	resp.insert(pos, clen_str);
// KEEP_ALIVE
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
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
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
	{
// RES_CGI_WAIT_COMPLETE
		conn->mod_evt(EPOLLOUT);
	}
	if ((this->done & RSRC_DONE_IO) == RSRC_DONE_IO)
		return (-1);
	return (0);
}
