/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/27 22:58:57 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"

volatile sig_atomic_t stop = 0;

static void sigint_handler(int signo)
{
    (void)signo;
	std::cerr << "SIGINT\n";
    stop = 1;
}

#if 0
https://man7.org/linux/man-pages/man7/epoll.7.html
https://man7.org/linux/man-pages/man2/epoll_ctl.2.html
#endif


// https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642

// The epoll_create system call returns a file descriptor to the newly created epoll kernel data structure. 

// When the EPOLL_CLOEXEC flag is set, any child process forked by the current process will close the epoll descriptor before it execs, so the child process won’t have access to the epoll instance anymore.

// Avoid forking, and if you must: close all epoll-registered file descriptors before calling execve. Explicitly deregister affected file descriptors from epoll set before calling dup/dup2/dup3 or close.

// https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/

// epoll_ctl(EPOLL_CTL_ADD) doesn't actually register a file descriptor. Instead it registers a tuple1 of a file descriptor and a pointer to underlying kernel object. Most confusingly the lifetime of an epoll subscription is not tied to the lifetime of a file descriptor. It's tied to the life of the kernel object.


Epoll::Epoll (void) : epfd(-1), ecnt(0)
{
	// this->epfd = epoll_create1(0); //  : execve will close this ... 
	this->epfd = epoll_create1(EPOLL_CLOEXEC); 
	if (this->epfd < 0)
		throw (std::runtime_error("Epoll : bad create"));
		
	signal(SIGINT, sigint_handler);

#if 0 // ILLEGAL FUNCTIONS
		// use this with wait_mask
		// wait_mask .. will REPLACE the current sigset
		// for the duration of the epoll_pwait() call
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &block_mask, NULL);
#endif
};

Epoll::Epoll(const Epoll & that) : epfd(that.epfd), ecnt(0)
{
	*this = that;
}

Epoll & Epoll::operator = (const Epoll & that)
{
	if (this == &that)
		return (*this);
	// UGLY : KILL SOMETHING HERE (?)
	this->epfd = that.epfd;
	return (*this);
}

Epoll::~Epoll()
{
	if (this->epfd != -1)
		close(this->epfd);
	
	std::set<EpollClient*>::iterator it = this->conn.begin();
	while (it != this->conn.end())
	{
		delete (*it); // Cgi Problem -- also .. epoll returns error
		it++;
	}
	this->conn.clear();
};


// track EpollClients here (?)
// CgiPipe .. 
	// TWO FD ? 
	// or
	// TWO different EpollClient
	// pipe
		// wfd : can we write (?) should have std::string& to write from
		// rfd : 
	// two pipes (?)
int	Epoll::add(EpollClient *cli, int e)
{
	int					err;
	struct epoll_event	evt;

	evt.events = e; // POLLIN / POLLOUT
	evt.events |= EPOLLRDHUP; // BOTH (in && rdhup) returned 
	// evt.events |= EPOLLET; // edge-triggered .. ONCE PER EVENT
	evt.data.ptr = cli;

	err = epoll_ctl(this->epfd, EPOLL_CTL_ADD, cli->get_fd(), &evt);
	if (err < 0)
	{
		std::cerr << "epoll : failed (add) fd " << cli->get_fd() << std::endl;
		if (cli->get_typ() != EPC_SERV)
			delete (cli);
	}
	else if (cli->get_typ() != EPC_SERV)
	{
		this->conn.insert(cli);
	}
	std::cerr << "epoll: add ";
	epc_type(cli);
	
	return (err);
}

int	Epoll::mod(EpollClient *cli, int e)
{
	int					err;
	struct epoll_event	evt; // should this be WITH EpollClient (?) ctl .. copies ..

	evt.events = e; // POLLIN / POLLOUT
	evt.events |= EPOLLRDHUP; // BOTH (in && rdhup) returned 
	evt.data.ptr = cli;

	err = epoll_ctl(this->epfd, EPOLL_CTL_MOD, cli->get_fd(), &evt);
	if (err < 0)
	{
		std::cerr << "epoll : failed (mod) fd " << cli->get_fd() << std::endl;
	}
	return (err);
}

#define DBG_EPOLL_DEL 0 
int	Epoll::del(EpollClient *cli)
{
	int err;

#if DBG_EPOLL_DEL
	std::cerr << "epoll : del " << cli->get_fd() << std::endl;
#endif
	err = epoll_ctl(this->epfd, EPOLL_CTL_DEL, cli->get_fd(), NULL);
	if (err < 0)
	{
		std::cerr << "epoll : failed (del) fd " << cli->get_fd() << std::endl;
	}
#if DBG_EPOLL_DEL
	std::cerr << "epoll : del " << cli->get_fd() << std::endl;
#endif
	return (err);
}

int	Epoll::rem(EpollClient *cli)
{
	if (cli->get_typ() == EPC_SERV)
		return (0);

#if DBG_EPOLL_DEL
	std::cerr << "epoll: rem CONN\n";
#endif
	std::set<EpollClient*>::iterator it = this->conn.find(cli);
	if (it != this->conn.end())
	{
#if DBG_EPOLL_DEL
		std::cerr << "epoll: rem CONN\n";
#endif			
		delete (cli);
		this->conn.erase(it);
	}

	return (0);
}

struct epoll_event	*Epoll::get_evt(int idx)
{
	if (idx < 0 || idx >= this->ecnt)
		return (NULL);
	return (this->evts + idx);
}

int	Epoll::loop(void)
{
	int					err;
	int					e;
	struct epoll_event	*evt;
	EpollClient 		*epc;
	
    while (!stop)
    {
        e = this->exec();
        if (e == 0)
		{
			// timeout
		}
        else if (e < 0)
        {
			// Epoll : FAIL
			// capture signal in exec (?)
			// CLEANUP (?)
			// restart (?)
			// copy fds (?)
			return (1);
		}
        while (e--) 
        {
			evt = this->get_evt(e);
			if (evt == NULL)
			{
				std::cerr << "epoll: evt NULL\n";
				continue;
			}
#if DBG_EPOLL
            evt_typ(evt);
#endif			
			epc = reinterpret_cast<EpollClient*>(evt->data.ptr);
			if (epc == NULL)
			{
				std::cerr << "epoll: epc NULL\n";
				continue;
			}
#if DBG_EPOLL
            epc_type(epc);
#endif		
            if (evt->events & EPOLLRDHUP)
            {
				std::cerr << "epoll: epc HUP\n";
#if 0                    
                int rfd = epc->get_fd();
                epc->pollout();
                // NOT QUITE -- we can still write to it ...
            	this->del(epc);
                close(rfd);
                // EOF 
                // CLOSE CONNECTION HERE

                continue;
#endif                
            }


            if (evt->events & EPOLLIN)
            {
				err = epc->pollin();
				if (err <= 0) // && state
				{
					// this->del(epc); // bad idea
					this->rem(epc);
				}
            }
            if (evt->events & EPOLLOUT)
            {
				err = epc->pollout();
				if (err <= 0) // && state ... 
				{
					this->rem(epc);
				}
			}
        }
    }
	return (0);
}


int	Epoll::exec(void)
{
#if 1
	this->ecnt = epoll_wait(this->epfd, this->evts, EPOLL_MAX_EVT, 1000); // to_ms
#else
// epoll_pwait() allows an application to safely
//        wait until either a file descriptor becomes ready or until a
//        signal is caught.

	sigset_t wait_mask;
    sigemptyset(&wait_mask); // mask for DURING epoll_pwait
// pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);
// ready = epoll_wait(epfd, &events, n, timeout);
// pthread_sigmask(SIG_SETMASK, &origmask, NULL);
	this->ecnt = epoll_pwait(this->epfd, this->evts, EPOLL_MAX_EVT, 1000, &wait_mask);
#endif
	if (this->ecnt < 0)
	{
		// SIGINT not "caught" .. if events are still waiting to process
		std::cerr << "epoll: < 0\n";
		std::cerr << strerror(errno) << std::endl;
		return (this->ecnt);
	}
	if (this->ecnt == 0) // timeout
	{
		return (0); // this->ecnt
	}
#if DBG_EPOLL
	std::cerr << "epoll : ecnt " << this->ecnt << std::endl;
#endif
	return (this->ecnt);
}

std::ostream& operator << (std::ostream & os, Epoll & obj)
{
	(void)obj;
	return (os);
}


// int epoll_ctl(int epfd, int op, int fd,
// struct epoll_event *_Nullable event); // is (event) .. "copied" -- I think so

// EPOLL_CTL_ADD
// EPOLL_CTL_MOD - changed .. existing (test?)
// EPOLL_CTL_DEL	

// edge-triggered mode delivers events only
// when changes occur on the monitored file descriptor, that is, an
// event will be generated upon each receipt of a chunk of data.

// Since the read operation done in 4
// does not consume the whole buffer data, the call to epoll_wait(2)
// done in step 5 might block indefinitely.

// Edge-triggered
// non-blocking
// waiting for an event only after read(2) or write(2) return EAGAIN.
// hm .. read-all-data .. not if we're getting sent chunks at a time (?)

// EPOLLIN		: can read
// EPOLLOUT		: can write
// EPOLLRDHUP	: peer closed OR shut down WRITE
	// detect close -- when using edge-tiggered
// EPOLLPRI		: exceptional condition (There is out-of-band data on a TCP socket)
// EPOLLERR		: always reported -- write end of pipe when read end is closed
// EPOLLHUP		: always reported -- hangup ;; peer closed its end -- may still have data to read

// Input Flags
// EPOLLET		: edge-triggered
// EPOLLONESHOT	:
// EPOLLWAKEUP	:
// EXPOLLECLUSIVE

void	evt_typ(struct epoll_event *evt)
{
	if (evt->events & EPOLLIN)
		std::cerr << "epoll : in\n";
	if (evt->events & EPOLLOUT)
		std::cerr << "epoll : out\n";
	if (evt->events & EPOLLRDHUP)
		std::cerr << "epoll : rdhup\n";
	if (evt->events & EPOLLPRI)
		std::cerr << "epoll : pri\n";
	if (evt->events & EPOLLERR)
		std::cerr << "epoll : err\n";
	if (evt->events & EPOLLHUP)
		std::cerr << "epoll : hup\n";
}




int	EpollClient::recv(void)
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
		this->state = EPC_STATE_ERROR;
		// this->shutdown();
		return (err);
	}
	if (err == 0)
	{
#if DBG_CONN_READ
		std::cerr << "conn  : read [0]\n";
#endif		
		this->state = EPC_STATE_SHUTDOWN;
		// this->shutdown();
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


void epc_type(EpollClient *epc)
{
	epc_typ t = epc->get_typ();
	if (t == EPC_SERV)
		std::cerr << "epc  : serv\n";
	if (t == EPC_CONN)
		std::cerr << "epc  : conn\n";
	if (t == EPC_CGI)
		std::cerr << "epc  : cgi\n";
}

int	EpollClient::send(std::string & buf)
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
		this->state = EPC_STATE_ERROR;
		// this->shutdown();
		return (err);
	}
	if (err == 0)
	{
#if DBG_CONN_WRITE
		std::cerr << "conn  : write [0]\n";
#endif
		this->state = EPC_STATE_SHUTDOWN;
		// this->shutdown();
		return (err);
	}
	buf.erase(0, err);

	// if (err == obuf.size())
	// {
		
	// }
	return (err);
}
