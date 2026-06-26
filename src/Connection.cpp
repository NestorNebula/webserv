/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 21:43:08 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"

Connection::Connection (int _fd, Server &_serv) : EpollClient(EPC_CONN, _fd), serv(_serv), 
	lact(0), 
	req_cnt(0)
{
};

Connection::Connection(const Connection & that) : EpollClient(EPC_CONN, that.fd), serv(that.serv), 
	lact(0),
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
	// NEED THIS : to implicitly "DEL" from (epoll)
	if (this->fd != -1)
		close(this->fd);
	// std::cerr << "~conn : req cnt " << this->req_cnt << std::endl;
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

int	Connection::shutdown(void)
{
	return (0);
	
	this->serv.ep.del(this);
	close(this->fd);
	this->fd = -1;
	return (0);
}


int	Connection::recv(void)
{
	int	err = 0;

	char	buf[CONN_BUF_SIZ + 1];
	err = read(this->fd, buf, CONN_BUF_SIZ);
#if DBG_CONN_READ
	std::cerr << "conn  : read\n";
	std::cerr << "read  : " << err << std::endl;
#endif
	if (err < 0)
	{
		this->state = CONN_STATE_ERROR;
		this->shutdown();
		return (err);
	}
	if (err == 0)
	{
#if DBG_CONN_READ
		std::cerr << "conn  : read [0]\n";
#endif		
		this->state = CONN_STATE_SHUTDOWN;
		this->shutdown();
		return (err);
	}
	buf[err] = '\0';
#if DBG_CONN_READ
	std::cerr << "****  : data\n" << buf << std::endl;
#endif	

	if (err == CONN_BUF_SIZ)
	{
		// more to read
	}

	// depends on state
	// Request
	// could also be reading from a cgi pipe (?)

	// when done / ready
	// MOD (epoll) to POLLOUT
	// make sure 
	// we are ready to write
	// and we have data (state) to write 
	ibuf += std::string(buf);
	return (err);
}

int	Connection::pollin(void)
{
	int	err = 0;

	this->timeout();
	
	err = this->recv();
	if (err <= 0)
		return (err);
		
#if DBG_CONN_READ
	std::cerr << "****  : ibuf\n" << ibuf << std::endl;
#endif

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
		
		obuf = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\nContent-Length: 8\r\n\r\nGotcha!\n");
		ibuf = std::string("");


		// Resource::generate()
			// may fail here .. bad file, directory
			
#if 1
		// int p[2];
		// err = pipe(p);
		if (err < 0)
		{
			return (-1);
		}
		pid_t pid = fork();
		if (pid == 0)
		{
			// dup2(fd, STDIN_FILENO);
			// close(STDIN_FILENO);
			dup2(fd, STDOUT_FILENO); // write directly .. 
			close(fd);
		// but .. they said we had to go through epoll

// CGI SPEC
		// https://www.ietf.org/rfc/rfc3875

			// chdir
			// env
			if (this->serv.get_port() == 8080)
			{
				char	bin[] = "/usr/bin/python";
				char	fil[] = "test.py";
				char * args[] = 
				{
					bin,
					fil,
					NULL
				};
				execve(bin, args, NULL);
			}
			else
			{
				char	bin[] = "/usr/bin/perl";
				char	fil[] = "test.pl";
				char * args[] = 
				{
					bin,
					fil,
					NULL
				};
				execve(bin, args, NULL);				
			}
		}
#endif

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

		waitpid(pid, 0, 0); // UGLY

		this->serv.ep.mod(this, EPOLLOUT);
}

	// CgiPipe
		// mode write ..
		// so .. pollwrite .. write FROM ibuf
		// and . pollread .. write TO obuf
			// which .. should get echo'ed .. directly .. to parent Connection (?)
	return (err);
}



int	Connection::send(std::string & buf)
{
	int err;
#if DBG_CONN_WRITE
	std::cerr << "conn  : pollout\n";
#endif

	size_t osiz = CONN_OUT_SIZ;
	if (osiz > buf.size())
		osiz = buf.size();
	err = write(this->fd, buf.c_str(), osiz);
	// BYTES WRITTEN !!

#if DBG_CONN_WRITE
	std::cerr << "conn  : write\n";
	std::cerr << "write : " << err << std::endl;
#endif	
	if (err < 0)
	{
		this->state = CONN_STATE_ERROR;
		this->shutdown();
		return (err);
	}
	if (err == 0)
	{
#if DBG_CONN_WRITE
		std::cerr << "conn  : write [0]\n";
#endif
		this->state = CONN_STATE_SHUTDOWN;
		this->shutdown();
		return (err);
	}
	buf.erase(0, err);

	// if (err == obuf.size())
	// {
		
	// }
	return (err);
}

int	Connection::pollout(void)
{
	int	err = 0;

	// test cgi state here (?)
	// backwards
	// if CgiPipeIn -- read from "parent" and write to cgi->input
	// if CgiPipeOut -- read from c
	// OR : write .. here .. checks if cgi has read data
	this->timeout();
	
	if (obuf.size() == 0)
	{
		// waiting for cgi data (?)
	}

	
	err = this->send(obuf);
	if (err <= 0)
		return (err);
	// but .. what if we want to come back and write more (?)
	// do we need to re-add .. MOD .. to kinda .. "reset" the event in the epoll
		
	// sure (?) pickup hup (?) before we get to write back (?)
	if (obuf.size() == 0)
	{
#if DBG_CONN_WRITE
		std::cerr << "conn  : write DONE\n";
#endif
		
#if 1 // KEEP_ALIVE
		// see more HUP => read [0] with THIS .. AND NOT siege.conf
		this->serv.ep.mod(this, EPOLLIN); // something more here ..
#else
		// fucking ep.del WORKED HERE ..
		// but not up in ep::loop 
		this->state = CONN_STATE_SHUTDOWN;
		this->shutdown();
		return (0);
#endif
	// If the close call removes the last pointer to kernel object and causes the object to be freed, then it will cause epoll subscription cleanup. But if there are more pointers to kernel object, more file descriptors, in any process on the system, then close will not cause the epoll subscription cleanup. It is totally possible to receive events on previously closed file descriptors.
	}
	return (err); // (!) bytes written
}

// hup : tell server to delete me (?)