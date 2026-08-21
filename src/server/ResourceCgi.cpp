/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:31:03 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 12:56:11 by kdonlon          ###   ########.fr       */
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
	WSLOG(LVL_DBG, TGT_RSRC, "resp: ", resp.size());

	// WSLOG(LVL_DBG, TGT_RSRC, "ostr");
	// WSLOG(LVL_DBG, TGT_RSRC, "****\n", ostr);
	
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
		
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "HEAD");
	// WSLOG(LVL_DBG, TGT_CGI_HEAD, "RESP:\n", resp);	
	this->hed = 1;
	
// REQUIRE (!)
	// Content-Type (?)
	std::string conn_close("Connection: close\r\n");
	resp.insert(0, conn_close);
	
	std::string stat_hed;
	std::string stat_str = hedval_str(resp, "Status");
	WSLOG(LVL_DBG, TGT_CGI_HEAD, "stat:  ", stat_str);
	if (stat_str.size())
	{
		// HTTP/1.1 STATUS [Status Message]
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
void	ResourceCgi::chk_rsp_len(void)
{
	std::string clen_str = hedval_str(resp, "Content-Length");
	if (clen_str.size())
	{
		// consider adding keep-alive
		return;
	}
	size_t	pos = resp.find("\r\n\r\n");
	std::string hed = resp.substr(0, pos + 4);
	size_t clen = (resp.size() - pos - 4);

	clen_str = std::string("\r\nContent-Length:") + num_2_str(clen);
	resp.insert(pos, clen_str);
}

void	ResourceCgi::set_err(int e)
{
	this->error = e;
	if (this->conn)
		this->conn->set_err(e);
}

int	ResourceCgi::set_done(int d)
{
	this->done |= d;
	if (this->done & RSRC_DONE_ERR)
		return (-1);
	if (this->done == RSRC_DONE_IO)
		return (-1);
	return (0);
}




// It is not currently possible to reliably delete epoll items when using the same epoll set from multiple threads. After calling epoll_ctl with EPOLL_CTL_DEL, another thread might still be executing code related to an event for that epoll item (in response to epoll_wait). Therefore the deleting thread does not know when it is safe to delete resources pertaining to the associated epoll item because another thread might be using those resources. 

// HM : do not delete ..CgiPipe .. until .. we are sure (cgi) is complete (?)
// so .. keep them in the epoll .. but .. disabled

// the reading application would be tied up for a long period; 
// meanwhile, it does not service I/O events on the other file descriptors—those descriptors are starved of service by the application. 

// The solution to file descriptor starvation is for the application to maintain a user-space data structure that caches the readiness of each of the file descriptors that it is monitoring. 

// so .. cache as (READY) .. until .. recv/send ZERO ...

// EPOLLONESHOT. If this flag is specified in the events mask for a file descriptor, then, once the file descriptor becomes ready and is returned by a call to epoll_wait(), it is disabled from further monitoring (but remains in the interest list). If the application is interested in monitoring file descriptor once more, then it must re-enable the file descriptor using the epoll_ctl(EPOLL_CTL_MOD) operation. 

// A defunct or "zombie" process in Linux occurs when a child process finishes running via fork(), but its parent process does not read its exit status using a wait() system call. The process remains in the table holding its PID until reaped.

