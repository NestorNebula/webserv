/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   EpollClient.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:21:06 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 16:51:25 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EPOLL_CLIENT_HPP
# define EPOLL_CLIENT_HPP

# include <unistd.h>
# include <string>
# include <vector>
# include "WsLog.hpp"


# ifndef EPC_BUF_SIZ
#  define EPC_BUF_SIZ 3 // 4095
# endif

# ifndef EPC_OUT_SIZ
#  define EPC_OUT_SIZ 3 // 4095 -- good test for cgi-out
# endif

// or .. EpollBufMgr
	// EpollBuf *hed;
	// EpollBuf *curW; // getW
		// Q: fill "partial" space 
		// or always create a new one
		// 
	// EpollBuf *curR; // getR : add new OR return one with avail
// Conn
	// reading a lot of file data to send to a cgi
	// conn::can_read ... read 
	// COULD -- not read any more until buf has been FULLY WRITTEN to cgi
	// OR : READ_MORE .. adding on to the linked list
	
	// also : we'd like to re-use as much as possible
	// if we read from the END ..
		// cleanup all "in-between"
	// and start writing to the (hed)
	// if read_from is not complete ...
		// the next write_to will have to add_on
// FifoBuf
	// linked list of chunks (EpollBuf)
class EpollBuf
{
public:
	char	*buf;
	
	EpollBuf(void) : buf(mem), nxt(NULL), beg(0), end(0), cnt(EPC_BUF_SIZ) {}
	
	char	*hed(void)
	{
		return (this->buf + this->beg);
	}
	ssize_t	rem(void)
	{
		return (this-> cnt - this->end);
	}
	ssize_t	siz(void)
	{
		if (this->end < this->beg)
			return (-1);
		return (this->end - this->beg);
	}
	// cleanup : assume (hed) static (?)
	// EpollBuf * get_write(void)
		// may need to allocate another
	// Epollbuf * get_read(void)
		// while (beg == end)
			// (nxt)
	// std::string str(void)
	// build from (potentially) multiple buffers
	// read_from_stream(cnt)
	// mostly .. fill/send one chunk at a time 
	// but this approach provides security
	// when not all bytes are read/sent
	// wrap (copy?)
	// push (char*)
	EpollBuf		*nxt;
private:
	char			mem[EPC_BUF_SIZ + 1]; // or :: FULL (for binary)
	ssize_t			beg;
	ssize_t			end;
	const ssize_t	cnt;
};

typedef enum
{
	EPC_SERV,
	EPC_CONN,
	EPC_CGI,
	EPC_MAX
}	epc_typ;
// ATTN : typ_str

typedef enum
{
	EPC_STATE_INIT,
	EPC_STATE_SHUTDOWN,
	EPC_STATE_ERROR,
	EPC_STATE_MAX
}	epc_state;

class Epoll;

class EpollClient
{
private:
	EpollClient & operator = (const EpollClient & ) 
		{ return (*this); }
public:
	EpollClient(Epoll & _ep, epc_typ _typ, int _fd);
	EpollClient (const EpollClient & that) : 
		ep(that.ep), typ(that.typ), fd(that.fd) {}
	virtual ~EpollClient();

	int			recv(void);
	// std::string -- fucks with binary data
	// std::vector<char> .. fucking ugly
	int			send(const char *buf, size_t siz);
	int			send(std::string & buf);
	
	virtual int	pollin(void) = 0;
	virtual int pollout(void) = 0;

	void		set_lact(void);
	
	int			get_fd(void)	const { return (this->fd); }
	epc_typ		get_typ(void)	const { return (this->typ); }
	epc_state	get_state(void)	const { return (this->state); }

    std::string typ_str(void);
	
	int			mod_evt(int e);
protected:
	Epoll		&ep;
	epc_typ		typ;
	int			fd;
	time_t		lact;

public:
	epc_state	state;
	int			error;
	
public:
	// EpollBuf		ebuf;
    char            ibuf[EPC_BUF_SIZ + 1];
	// size_t		isiz;
	std::string		istr;

	// hm : we have to imagine this is coming from somewhere else ...
	std::string		ostr;
	// char            obuf[]
	// size_t		osiz;
};

#endif