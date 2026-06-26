/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 10:32:05 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"

Epoll::Epoll (void) : epfd(-1)
{
	this->epfd = epoll_create1(0); // EPOLL_CLOEXEC
	if (this->epfd < 0)
		throw (std::runtime_error("Epoll : bad create"));
};

Epoll::Epoll(const Epoll & that) : epfd(-1)
{
	*this = that;
}

Epoll & Epoll::operator = (const Epoll & that)
{
	if (this == &that)
		return (*this);
	// KILL : 
	this->epfd = that.epfd;
	return (*this);
}

Epoll::~Epoll()
{
	// remove all connection (?)
	// do we need to track them elsewhere (?)
	// 
	if (this->epfd != -1)
		close(this->epfd);
	
};

// EpollClient * .. WOULD NEED protected (fd)
int	Epoll::add(int fd, int e, void *data)
{
	int err;

	struct epoll_event evt;

	evt.events = e; // POLLIN / POLLOUT
	evt.events |= EPOLLRDHUP; // BOTH (in && rdhup) returned 
	// evt.events |= EPOLLET; // edge-triggered .. ONCE PER EVENT
	evt.data.ptr = data;

	err = epoll_ctl(this->epfd, EPOLL_CTL_ADD, fd, &evt);
	if (err < 0)
	{
		std::cerr << "epoll : failed ctl\n";
	}
	return (err);
}

int	Epoll::mod(int fd, int e, void *data)
{
	int err;

	struct epoll_event evt;

	evt.events = e; // POLLIN / POLLOUT
	evt.events |= EPOLLRDHUP; // BOTH (in && rdhup) returned 
	evt.data.ptr = data;

	err = epoll_ctl(this->epfd, EPOLL_CTL_MOD, fd, &evt);
	if (err < 0)
	{
		std::cerr << "epoll : failed ctl\n";
	}
	return (err);
}

int	Epoll::del(int fd)
{
	int err;

	err = epoll_ctl(this->epfd, EPOLL_CTL_DEL, fd, NULL);
	if (err < 0)
	{
		std::cerr << "epoll : failed ctl\n";
	}
	return (err);
}

struct epoll_event	*Epoll::get_evt(int idx)
{
	if (idx < 0 || idx >= this->ecnt)
	{
		return (NULL);
	}
	return (this->evts + idx);
}

int	Epoll::loop(void)
{
	int					err;
	int					e;
	struct epoll_event	*evt;
	EpollClient 		*epc;
	
    while (1)
    {
        e = this->exec();
        if (e == 0)
            ; // timeout
        else if (e < 0)
        {
			// Epoll : FAIL
			// CLEANUP
			return (1);
		}
        while (e--) 
        {
            // struct epoll_event	*
			evt = this->get_evt(e);
			if (evt == NULL)
			{
				std::cerr << "epoll: evt NULL\n";
				continue;
			}
            evt_typ(evt);
			epc = reinterpret_cast<EpollClient*>(evt->data.ptr);
            if (evt->events & EPOLLRDHUP)
            {
                // ep.del() // need fd .. 
                // remove "Connection" from "Server"
                // ep.hup .. 
                // then .. delete it -- is that a good idea (?)
                // or .. delete Connection -- 
                    // removes itself from the Server in destructor ..
                    // assumes : Server still exists (?)
                // POLLIN -- also set .. but ... 
                // read returns (0) .. so .. 
                // ready-to-read .. but returns (0) .. means
                // EOF reached on INPUT 
#if 0                    
                int rfd = epc->get_fd();
                epc->pollout();
                // NOT QUITE -- we can still write to it ...
                ep.del(rfd);
                close(rfd);
                // EOF 
                // CLOSE CONNECTION HERE

                continue;
#endif                
            }


            if (evt->events & EPOLLIN)
            {
                if (epc)
                {
                    err = epc->pollin();
					(void)err;
					// or : check something to decide what to do here .. 
					// nothing more to read .. may still have somethign to write
                }
            }
            if (evt->events & EPOLLOUT)
            {
                if (epc)
                {
                    err = epc->pollout();
					(void)err;
                }
            }
        }
    }
	return (0);
}
int	Epoll::exec(void)
{
	// struct timespec	epto;
	// sigset_t			sigs;
	
	this->ecnt = epoll_wait(this->epfd, this->evts, EPOLL_MAX_EVT, 1000); // to_ms
	// err = epoll_pwait2(this->epfd, this->evts, EPOLL_MAX_EVT,
// int epfd, struct epoll_event events[n], int n,
// const struct timespec *_Nullable timeout,
// const sigset_t *_Nullable sigmask);
	if (this->ecnt < 0)
	{
		return (this->ecnt);
	}
	if (this->ecnt == 0)
	{
		// TIMEOUT -- something to do here (?)
		// check state .. of all active fds 
		return (0);
	}
	std::cerr << "epoll : ecnt " << this->ecnt << std::endl;
	// if (serv) no accept ... 
	// keeps getting called 
	// with EPOLLET -- only ONE EVENT triggered 
	return (this->ecnt);
}

// The ready list is dynamically populated by the kernel as
// a result of I/O activity on those file descriptors.

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

// struct epoll_event events[MAX_EVENTS];

// int epoll_pwait2(int n;
// int epfd, struct epoll_event events[n], int n,
// const struct timespec *_Nullable timeout,
// const sigset_t *_Nullable sigmask);



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