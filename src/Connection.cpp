/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 12:09:48 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"
#include "CgiPipe.hpp"

Connection::Connection (int _fd, Server &_serv) : EpollClient(EPC_CONN, _fd), serv(_serv), 
	req_cnt(0), filedes(-1)
{
};

Connection::Connection(const Connection & that) : EpollClient(EPC_CONN, that.fd), serv(that.serv),
	req_cnt(0), filedes(-1)
{
}


Connection::~Connection()
{
	WsLog::_(LVL_DBG, TGT_CONN, "(~) Connection");
	// WsLog::_(LVL_DBG, TGT_CONN, "req cnt: ", this->req_cnt);
	if (filedes != -1) // Resource
		close(filedes);
};

// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 
// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 
// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 

int	Connection::pollin(void)
{
	int	err = 0;

	this->timeout();
	
	err = this->recv();
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv");
		return (err);
	}
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_RECV, "recv: zero");
		return (-1);
	}


// EpollBuf

	// copy (?) or keep-adding (?)
	istr += std::string(this->ibuf);
    ivec.insert(ivec.end(), ibuf, ibuf + err);
	
	// WsLog::_(LVL_INFO, TGT_CONN_RECV, "istr");
	// WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);
	

	std::string hed_end("\r\n\r\n");
	
	if (istr.find(hed_end) == std::string::npos)
		return (err);

	this->req_cnt++;

	WsLog::_(LVL_INFO, TGT_CONN_RECV, "istr");
	WsLog::_(LVL_INFO, TGT_CONN_RECV, "\n", istr);
	// for (size_t k=0; k < ivec.size(); k++)
	// 	std::cerr << ivec[k];
		
	
	// erase: up to \r\n\r\n
	istr = std::string("");



#define RESP_SIMPLE 0
#define RESP_FILE 1
#define RESP_CGI 2
#define RESP 2


#if (RESP == RESP_FILE)
	filedes = open("./2k_earth_daymap.jpg", O_RDONLY);
	if (filedes < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "open (file)");
	ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: image/jpg\r\nContent-Length: 463087\r\n\r\n");

	// filedes = open("./Kanan.mp3", O_RDONLY);
	// if (filedes < 0)
	// 	return WsLog::_errno(LVL_ERR, TGT_CONN, "open (file)");
	// ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: audio/mp3\r\nContent-Length: 14975750\r\n\r\n");
	
	// GENERAL QUESTION : send-buf-until-flushed (?)
	// one "send" per "epoll" (?)

	this->send(ostr); // UGLY : testing
	this->serv.ep.mod(this, EPOLLOUT);
	
#elif (RESP == RESP_CGI)

	err = this->exec_cgi();
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_CONN, "exec_cgi");
		return (err);
	}

	// ATTN : may have more to read ... 
	// to send to cgi-stdin
	this->serv.ep.mod(this, 0);
	
	// Resource::generate()
		// may fail here .. bad file, bad directory, bad pipe, not allowed
		
// the CONTENT_LENGTH environment variable needs to be set

// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
	// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read. 
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
	// wow : open cgi file and write TO stdin (?)
	

	// may get more body here 

	// need to know cgi INPUT fd .. to write to it (?)
	// OR : CGI .. "reads" from conn->istr
#else //  (RESP == RESP_SIMPLE)
// MINI_BODY
	ostr = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 9\r\n\r\nGotcha!!\n");
	this->send(ostr); // not here .. but TEST 
	this->serv.ep.mod(this, EPOLLOUT);
#endif
	return (err);
}


// ∗ Just remember that, for chunked requests, your server needs to un-chunk
// them, the CGI will expect EOF as the end of the body.
// ∗ The same applies to the output of the CGI. If no content_length is
// returned from the CGI, EOF will mark the end of the returned data.
// ∗ The CGI should be run in the correct directory for relative path file access.

int	Connection::pollout(void)
{
	int	err = 0;

	this->timeout();

#if (RESP == RESP_FILE)

	// and .. header (ostr) needs to be sent .. 
	// MsgBuf .. was pretty handy
	// output of CGI may be binary as well 
	if (filedes > 0)
	{
		err = read(filedes, this->ibuf, EPC_BUF_SIZ);
		if (err < 0)
		{
			close(filedes);
			filedes = -1;
			return WsLog::_errno(LVL_ERR, TGT_CONN, "read (file)");
		}
		if (err == 0)
		{
			close(filedes);
			filedes = -1;
			WsLog::_(LVL_DBG, TGT_CONN_RECV, "read: zero");
			return (-1);
		}
		err = this->send(ibuf, err);

		// and if NOT ALL BYTES ARE SENT (?)
		if (err < 0)
			return WsLog::_errno(LVL_ERR, TGT_CONN_SEND, "send");
		if (err == 0)
		{
			WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: zero");
			return (-1);
		}
		
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);

	}
#else

	// v.insert(v.end(), data2, data2 + strlen(data2));

	// CGI && SIMPLE .. "filled" (ostr)
	// Is this a good idea (?)
	if (ostr.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: ostr.size() == 0");
		this->serv.ep.mod(this, 0);
		return (0);
	}

	err = this->send(ostr);


	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN_SEND, "send");
	if (err == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: zero");
		return (-1);
	}
	
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: ", err);
	// WsLog::_(LVL_DBG, TGT_CONN_SEND, ostr);

	
	if (ostr.size())
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "left: ", ostr.size());
		// WsLog::_(LVL_DBG, TGT_CONN_SEND, ostr);
	}
	else
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent: all");
		
		// Q: have we sent (content-length) bytes (?)
		// Q: chunked send .. must close 
		// this->serv.ep.mod(this, 0);
		// nothing if that was the header for a FILE 

#if 0 // KEEP_ALIVE
		// see more HUP => read [0] with THIS .. AND NOT siege.conf
		this->serv.ep.mod(this, EPOLLIN); // something more here ..
#else
		// TEST : content-length header .. longer than what we send
		// curl: (18) end of response with 5 bytes missing
		return (-1);		
#endif
	}

#endif
	return (err); // (!) bytes written
}





int	Connection::exec_cgi(void)
{
	int			err;

	std::string	path;
	std::string	file;
	
	if (false) // this->serv.get_port() == 8080)
	{
		path = std::string("/usr/bin/python");
		file = std::string("test.py");
	}
	else
	{
		// path = std::string("/usr/bin/perl");
		// file = std::string("test.pl");

		path = std::string("/usr/bin/php-cgi"); // DIFFERENT
		file = std::string("test.php");
	}

	int		p1[2];
	int		p2[2];

	err = pipe(p1);
	if (err < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipe");
	err = pipe(p2);
	if (err < 0)
	{
		// cleanup
		return WsLog::_errno(LVL_ERR, TGT_CONN, "pipe");
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		// cleanup
		return WsLog::_errno(LVL_ERR, TGT_CONN, "fork");
	}
	if (pid == 0)
	{
		// Epoll:: CLOEXEC should have closed Epoll::epfd

		close(p1[1]);
		err = dup2(p1[0], STDIN_FILENO);
		if (err < 0)
		{
			// cleanup
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p1[0]);
		
		close(p2[0]);
		err = dup2(p2[1], STDOUT_FILENO);
		if (err < 0)
		{
			// cleanup
			return WsLog::_errno(LVL_ERR, TGT_CONN, "dup2");
		}
		close(p2[1]);

		char const *args[3];
		args[0] = path.c_str();
		args[1] = file.c_str();
		args[2] = NULL;

		// std::string qs("QUERY_STRING=p1=one&p2=two");
		// char const *envp[2];
		// envp[0] = qs.c_str();
		// envp[1] = NULL;

		// for POST - need to have CONTENT_LENGTH in (env)
		// and then .. send data to cgi-stdin
		std::string cl("CONTENT_LENGTH=13");
		char const *envp[2];
		envp[0] = cl.c_str();
		envp[1] = NULL;

		// so .. read FROM cgi .. until (ostr) is full
		// then we can slap the headers on (?)

		err = execve(args[0], (char* const*) args,
			// NULL);
			(char* const*) envp);
		std::cout << "\n\nwtf\n\n";
		return (err);	
	}		
	
	WsLog::_(LVL_INFO, TGT_CONN, "exec cgi");

	close(p1[0]);
	close(p2[1]);

	int			cgifd;
	EpollClient	*epc_cgi;


	// Q: need to track (!)
	// .. to write to (?)
	// or .. 
	// we just always .. read to (ibuf)
	// then "signal" the cgi's pollout .. that it should check 
	// for something new to read .. 
#if 0
	cgifd = dup(p1[1]);
	if (cgifd < 0)
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	close (p1[1]);

	epc_cgi = new CgiPipe(cgifd, *this);
	err = this->serv.ep.add(epc_cgi, EPOLLOUT);
	if (err < 0)
		return (err);
		
#else
	close(p1[1]);
#endif


	cgifd = dup(p2[0]);
	if (cgifd < 0)
	{
		return WsLog::_errno(LVL_ERR, TGT_CONN, "dup");
	}
	close (p2[0]);
	
	epc_cgi = new CgiPipe(cgifd, *this);
	err = this->serv.ep.add(epc_cgi, EPOLLIN);
	if (err < 0)
	{
		// something to do (?)
		return (err);
	}
	return (err);
}








