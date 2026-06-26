/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:35 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/21 14:38:49 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"
#include "Server.hpp"

Connection::Connection (void) : fd(-1), serv(NULL)
{
};

Connection::Connection (int _fd, Server *_serv) : fd(_fd), serv(_serv)
{
};

Connection::Connection(const Connection & that) : fd(that.fd), serv(that.serv)
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
	if (this->fd != -1)
		close(this->fd);
};


std::ostream& operator << (std::ostream & os, Connection & obj)
{
	(void)obj;
	return (os);
}



int Connection::get_fd(void)
{
	return (this->fd);
}

#define CONN_BUF_SIZ 2
// so : we will keep getting POLLIN events .. 
// until there is nothing more to read ...

int	Connection::pollin(void)
{
	int	err = 0;

	// update_active_time
	
	char	buf[CONN_BUF_SIZ + 1];
	err = read(this->fd, buf, CONN_BUF_SIZ);
	std::cerr << "conn : read\n";
	std::cerr << "read : " << err << std::endl;
	if (err < 0)
	{
		return (err);
	}
	buf[err] = '\0';
	std::cerr << "data\n" << buf << std::endl;
	if (err == 0)
	{
		// hangup (?)
		// does getting here .. really mean .. EOF
		// yeah .. done .. on this end ..
		// whether we shut down
		// depeds on what else we have to do
		return (err);
	}
	if (err == CONN_BUF_SIZ)
	{
		// need more 
	}

	// depends on state
	// Request
	// could also be reading from a cgi pipe (?)

	// when done / ready
	// MOD (epoll) to POLLOUT
	// make sure 
	// we are ready to write
	// and we have data (state) to write 
	ibuf += std::string(buf); // NOT NULL TERMINATED

	// check for valid header (??)
	// may still have (body) to read after ...
	// TEST : until ibuf size  > 20

	// ep.mpd .. 
	// attn : rest of buf lost 
	// may still have more to read .. 
	if (ibuf.size() > 20)
	{
		// Parse Request
		// 1) read file to (obuf)
		// 2) generate directory listing to (obuf)
		// 3) fork (cgi) .. which is within a loop here .. 
			// 
		// 4) may be UPLOADING FILE DATA
		// read_file(obuf)
		// gen_dir(obuf)
		obuf = ibuf;
		ibuf = std::string("");
		this->serv->ep.mod(this->fd, EPOLLOUT, this);
	}
	// are we sure we're in there (?)
	// does MOD on (fd) not currently in set .. error (?)
	return (err);
}

int	Connection::pollout(void)
{
	int	err = 0;

	// update_active_time


	std::cerr << "conn : pollout\n";

	// only write .. max_bytes per call 
	// if we are uploading a large file
	// or receiving from cgi
	
	err = write(this->fd, obuf.c_str(), obuf.size());

	std::cerr << "conn : write\n";
	std::cerr << "write: " << err << std::endl;
	if (err < 0)
	{

	}
	if (err != (int) obuf.size())
	{
		
	}
	// what .. now we HAVE to write something (?)
	
	// must write something .. yes (?)
	// but .. what if we want to come back and write more (?)
	// do we need to re-add .. MOD .. to kinda .. "reset" the event in the epoll
		
	// sure (?) pickup hup (?) before we get to write back (?)
	this->serv->ep.mod(this->fd, EPOLLIN, this);

	return (err);
}

// hup : tell server to delete me (?)