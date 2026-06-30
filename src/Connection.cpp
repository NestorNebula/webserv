/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 15:35:20 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"

Connection::Connection (int _fd, Server &_serv) : EpollClient(EPC_CONN, _fd), serv(_serv), 
	lact(0), 
	cgi_in(NULL),
	cgi_out(NULL),
	req_cnt(0)
{
};

Connection::Connection(const Connection & that) : EpollClient(EPC_CONN, that.fd), serv(that.serv), 
	lact(0),
	cgi_in(NULL),
	cgi_out(NULL),
	req_cnt(0)
{
}

Connection & Connection::operator = (const Connection & that)
{
	if (this == &that)
		return (*this);
	// FREE DATA : this
	// COPY DATA : this->val = that.val
	return (*this);
}
Connection::~Connection()
{
	// if (this->fd != -1)
	// 	close(this->fd);
	// std::cerr << "~conn  : req cnt " << this->req_cnt << std::endl;
	
// LET (epoll) take care of this (?)

	// if (this->cgi_in)
	// 	delete (this->cgi_in);
	// if (this->cgi_out)
	// 	delete (this->cgi_out);
};


std::ostream& operator << (std::ostream & os, Connection & obj)
{
	(void)obj;
	return (os);
}

#define CONN_TO_SECS 5

int	Connection::timeout(void)
{
	int	err = 0;

	time_t now = time(&now);
	if (this->lact)
	{
		double secs = ((double) (now - lact));
		if (secs > CONN_TO_SECS)
		{
			std::cerr << "conn  : timed out\n";
			err = 1;
		}
	}
	this->lact = now;
	return (err);
}

#if 0
int	Connection::shutdown(void)
{
	return (0);
	
	this->serv.ep.del(this);
	close(this->fd);
	this->fd = -1;
	return (0);
}
#endif

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
		this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}

	// may may be writing this data to an UPLOAD file .. 

	// Resource -- CGI .. create here (?) or in Epoll::loop()
		// add sides of pipe to epoll 
		// what happens when it gets forked (?)
	
	std::string hed_end("\r\n\r\n");
if (ibuf.find(hed_end) != std::string::npos)
{
// ATTN : KEEP_ALIVE
// Warning: Connection-specific header fields such as Connection and Keep-Alive are prohibited in HTTP/2 and HTTP/3. 
		this->req_cnt++;
		
			// empty reply -- but .. something is sent (?

		ibuf = std::string("");
#if 1
		obuf = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 8\r\n\r\nGotcha!\n");

		this->serv.ep.mod(this, EPOLLOUT);
#else // CGI
		err = this->exec_cgi();
		if (err)
		{
			std::cerr << "conn  : exec cgi error " << strerror(errno) << std::endl;
			return (err);
		}
		// Resource::generate()
			// may fail here .. bad file, directory
			
		// so .. maybe .. CgiPipe .. is NOT an EPOLL_CLIENT
		// CgiInput
			// pollin
			// write from (ibuf) of (conn)
		// CgiOutput
			// pollout
			// write to (obuf) of conn
			
// multipart/form-data : cgi would need to know the BOUNDARY in the HEADER
		// write rest of BODY to cgi->ifd;
// 		A request-body is supplied with the request if the CONTENT_LENGTH is
//    not NULL.  The server MUST make at least that many bytes available
//    for the script to read. 
// The script MUST check the value of the CONTENT_LENGTH variable before
//    reading the attached message-body, and SHOULD check the CONTENT_TYPE
//    value before processing it
		// wow : open cgi file and write TO stdin (?)
		
		// cgi->ofd : should be added to (epoll) ?
		
		// may get more body here 

			// crash without this 
			// gets .. deleted (?)
		// or >> (DEL) .. until cgi has written stuff
#endif
}

	// CgiPipe
		// mode write ..
		// so .. pollwrite .. write FROM ibuf
		// and . pollread .. write TO obuf
			// which .. should get echo'ed .. directly .. to parent Connection (?)
	return (err);
}

int	Connection::pollout(void)
{
	int	err = 0;

	WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
	
	// test cgi state here (?)
	// backwards
	// if CgiPipeIn -- read from "parent" and write to cgi->input
	// if CgiPipeOut -- read from c
	// OR : write .. here .. checks if cgi has read data
	this->timeout();
	if (obuf.size() == 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send: obuf.size() == 0");
		this->serv.ep.del(this);
		return (0);
	}

	err = this->send(obuf);
	if (err < 0)
	{
		WsLog::_(LVL_DBG, TGT_CONN_SEND, "send");
		return (-1); // err);
	}
	if (err == 0)
	{
		this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}
	WsLog::_(LVL_DBG, TGT_CONN_SEND, "sent");
	WsLog::_(LVL_DBG, TGT_CONN_SEND, obuf);
	// but .. what if we want to come back and write more (?)
	// do we need to re-add .. MOD .. to kinda .. "reset" the event in the epoll
		
	// sure (?) pickup hup (?) before we get to write back (?)
	if (obuf.size() == 0)
	{
#if 0 // KEEP_ALIVE
		// see more HUP => read [0] with THIS .. AND NOT siege.conf
		this->serv.ep.mod(this, EPOLLIN); // something more here ..
#else
		// fucking ep.del WORKED HERE ..
		// but not up in ep::loop 
		// this->state = EPC_STATE_SHUTDOWN;
		// this->shutdown();
		// return (err);
#endif
	// If the close call removes the last pointer to kernel object and causes the object to be freed, then it will cause epoll subscription cleanup. But if there are more pointers to kernel object, more file descriptors, in any process on the system, then close will not cause the epoll subscription cleanup. It is totally possible to receive events on previously closed file descriptors.
	}
	return (err); // (!) bytes written
}

// hup : tell server to delete me (?)



int	Connection::exec_cgi(void)
{
	int			err;

	std::string	path;
	std::string	file;
	
	if (this->serv.get_port() == 8080)
	{
		path = std::string("/usr/bin/python");
		file = std::string("test.py");
	}
	else
	{
		path = std::string("/usr/bin/perl");
		file = std::string("test.pl");
	}

	int		p1[2];
	int		p2[2];

	err = pipe(p1);
	if (err < 0)
		return (err); // return WsLog::_err(TGT, "pipe")
	err = pipe(p2);
	if (err < 0)
		return (err);


	pid_t pid = fork();
	if (pid < 0)
	{
		return (-1);
	}
	if (pid == 0)
	{
		// this->serv.ep.copied(); // UGLY

		close(p1[1]);
		err = dup2(p1[0], STDIN_FILENO);
		if (err < 0)
			return (err);
		close(p1[0]);
		
		close(p2[0]);
		err = dup2(p2[1], STDOUT_FILENO);
		if (err < 0)
			return (err);
		close(p2[1]);

		// const char *args[3];
		char const * args[3];
		args[0] = path.c_str();
		args[1] = file.c_str();
		args[2] = NULL;

		// ah -- not seeing this 
		std::cerr << "\nFORK  : exec\n";
// sys.excepthook is missing
// lost sys.stderr
		// close(STDERR_FILENO); 
		// is (envp) important (?)
		err = execve(args[0], (char* const*) args, NULL);
		std::cout << "\n\nwtf\n\n";
		return (err);	
	}		
	close(p1[0]);
	close(p2[1]);

		// epoll error (?)
	this->cgi_in  = new CgiPipe(p1[1], *this);
	this->cgi_out = new CgiPipe(p2[0], *this);

	err = this->serv.ep.add(this->cgi_in, EPOLLOUT);
	if (err < 0)
		return (err);
	err = this->serv.ep.add(this->cgi_out, EPOLLIN);
	if (err)
		return (err);
	std::cerr << "\nconn  : exec_cgi\n";
	// why TWO here ?
	this->serv.ep.mod(this, 0);
	return (err);
}










CgiPipe::CgiPipe (int _fd, Connection & _conn) : EpollClient(EPC_CGI, _fd), conn(_conn)
{
}
	
int		CgiPipe::pollin(void)
{
	int	err = 0;

	// this->timeout(); // EpollClient
	
	err = this->recv();
	std::cerr << "cgi   : pollin " << err << std::endl;
	if (err < 0)
	{

		return (err);
	}
	std::cerr << "data\n" << this->ibuf << std::endl;
	// this (fd) .. can READ
	// output from CGI (?)
	// to conn.obuf
	// we ARE GETTING THE DATA ... 
	// tell our "parent" Conn we can start writing
		
	this->conn.obuf = this->ibuf;
	// which we should cleanup
	// or .. have written to to begin with
	// add as one-shot (?)

	// hm .. 
	this->conn.serv.ep.mod(&this->conn, EPOLLOUT);

	return (0);
}

int		CgiPipe::pollout(void)
{
	// this (fd) .. can WRITE
	// input TO CGI .. from conn.ibuf
	// WsLog::_(LVL_DBG, TGT_CGI_SEND, "pollout");
	
	this->conn.serv.ep.del(this);
	// maybe .. clsing the (fd) .. is enough
	// so .. mod(0) .. would "silence" ... 
	// but .. keep error (?)
	// this->conn.serv.ep.rem(this); /// SegFault
	return (-1); // remove -- SegFAULT
}
