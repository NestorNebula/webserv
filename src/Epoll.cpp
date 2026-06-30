/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 15:35:11 by kdonlon          ###   ########.fr       */
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

static const char *evt_name[] =
{
	"in ",
	"out ",
	"rdhup ",
	"pri ",
	"err ",
	"hup ",
	NULL
};

// MULTIPLE (1)
static std::string evt_type(struct epoll_event *evt)
{
	std::string typ;
	if (evt->events & EPOLLIN)
		typ += (evt_name[0]);
	if (evt->events & EPOLLOUT)
		typ += (evt_name[1]);
	if (evt->events & EPOLLRDHUP)
		typ += (evt_name[2]);
	if (evt->events & EPOLLPRI)
		typ += (evt_name[3]);
	if (evt->events & EPOLLERR)
		typ += (evt_name[4]);
	if (evt->events & EPOLLHUP)
		typ += (evt_name[5]);
	return (typ);
}


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

void	Epoll::copied(void)
{
	std::set<EpollClient*>::iterator it = this->conn.begin();
	while (it != this->conn.end())
	{
		close ( (*it)->get_fd());
	}
}

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

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "add client: ", epc_type(cli));
	err = epoll_ctl(this->epfd, EPOLL_CTL_ADD, cli->get_fd(), &evt);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "add failed: ", strerror(errno));
		
		// if (cli->get_typ() != EPC_SERV)
			delete (cli);
	}
	else // if (cli->get_typ() != EPC_SERV)
	{
		this->conn.insert(cli);
	}

	
	return (err);
}

int	Epoll::mod(EpollClient *cli, int e)
{
	int					err;
	struct epoll_event	evt; // should this be WITH EpollClient (?) ctl .. copies ..

	evt.events = e; // POLLIN / POLLOUT
	evt.events |= EPOLLRDHUP; // BOTH (in && rdhup) returned 
	evt.data.ptr = cli;

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "mod client: ", epc_type(cli));
	err = epoll_ctl(this->epfd, EPOLL_CTL_MOD, cli->get_fd(), &evt);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "mod failed: ", strerror(errno));	
	}
	return (err);
}

int	Epoll::del(EpollClient *cli)
{
	int err;

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "del client: ", epc_type(cli));
	err = epoll_ctl(this->epfd, EPOLL_CTL_DEL, cli->get_fd(), NULL);
	if (err < 0)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "del failed: ", strerror(errno));
	}
	return (err);
}

int	Epoll::rem(EpollClient *cli)
{
	// if (cli->get_typ() == EPC_SERV)
	// 	return (0);

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "rem client: ", epc_type(cli));
	std::set<EpollClient*>::iterator it = this->conn.find(cli);
	if (it != this->conn.end())
	{
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
		WsLog::_(LVL_ERR, TGT_EPOLL, "wait: ", strerror(errno));
		return (-1);
	}
	if (this->ecnt == 0) // timeout
	{
		return (0);
	}

	WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "\n");
	WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "ecnt");
	WsLog::_(LVL_DBG, TGT_EPOLL_EVT, this->ecnt);
	
	return (this->ecnt);
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
		for (int k=0; k < e; k++) 
        {
			evt = this->get_evt(k);
			if (evt == NULL)
			{
				WsLog::_(LVL_WARN, TGT_EPOLL_EVT, "evt NULL");
				continue;
			}
			epc = reinterpret_cast<EpollClient*>(evt->data.ptr);
			if (epc == NULL)
			{
				WsLog::_(LVL_WARN, TGT_EPOLL_EVT, "epc NULL");
				continue;
			}
			// had event .. on something we deleted 
// Cgi SegFault .. deleted (?)
			WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt target: ", epc_type(epc));

			WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt type  : ", evt_type(evt));
		
            if (evt->events & EPOLLHUP)
			{
				this->rem(epc);
				continue;
			}
            if (evt->events & EPOLLIN)
            {
				err = epc->pollin();
				if (err < 0) // && state
				{
					// this->del(epc); // bad idea
					this->rem(epc);
					continue;
				}
            }
            if (evt->events & EPOLLOUT)
            {
				err = epc->pollout();
				if (err < 0) // && state ... 
				{
					// this->del(epc); // bad idea
					this->rem(epc);
					continue;
				}
			}
			if (evt->events & EPOLLRDHUP)
			{
				this->rem(epc);
				continue;
			}
        }
    }
	return (0);
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




// SegFault -- if deleted (?) by whom (?) forked process (?)
static const char *epc_str[] = 
{
	"serv",
	"conn",
	"cgi",
	NULL
};

const char *epc_type(EpollClient *epc)
{
	epc_typ t = epc->get_typ();
	return (epc_str[t]);
}


int	EpollClient::recv(void)
{
	int	err = 0;

	char	buf[CONN_BUF_SIZ + 1];
	err = read(this->fd, buf, CONN_BUF_SIZ);
	
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "recv");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, err);
	
	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		WsLog::_(LVL_ERR, TGT_EPC_RECV, "recv: ", strerror(errno));
		// this->shutdown();
		return (err);
	}
	if (err == 0)
	{
		this->state = EPC_STATE_SHUTDOWN;
		// this->shutdown();
		return (-1);
	}
	buf[err] = '\0';

	WsLog::_(LVL_INFO, TGT_EPC_RECV, "data");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "\n", buf);

	if (err == CONN_BUF_SIZ)
	{
		// more to read
		// epoll should let us know 
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
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "ibuf");
	WsLog::_(LVL_INFO, TGT_EPC_RECV, "\n", ibuf);
	
	return (err);
}

int	EpollClient::send(std::string & buf)
{
	int err;
	
	WsLog::_(LVL_INFO, TGT_EPC_SEND, "send");
	WsLog::_(LVL_INFO, TGT_EPC_SEND, buf.size());

	size_t osiz = CONN_OUT_SIZ;
	if (osiz > buf.size())
		osiz = buf.size();
	err = write(this->fd, buf.c_str(), osiz);

	WsLog::_(LVL_INFO, TGT_EPC_SEND, "sent");
	WsLog::_(LVL_INFO, TGT_EPC_SEND, err);

	if (err < 0)
	{
		this->state = EPC_STATE_ERROR;
		WsLog::_(LVL_ERR, TGT_EPC_SEND, "send: ", strerror(errno));
		// this->shutdown();
		return (err);
	}
	if (err == 0)
	{
		this->state = EPC_STATE_SHUTDOWN;
		return (-1);
	}
	buf.erase(0, err);

	// if (err == obuf.size())
	// {
		
	// }
	return (err);
}
