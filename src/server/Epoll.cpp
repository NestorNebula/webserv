/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/07 21:27:53 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include "EpollClient.hpp"

#include "Server.hpp"
#include "Connection.hpp"

volatile sig_atomic_t stop = 0;

static void sigint_handler(int signo)
{
    (void)signo;
	
	WsLog::_(LVL_ERR, TGT_EPOLL, "");
	WsLog::_(LVL_ERR, TGT_EPOLL, "SIGINT");

	// *** CGI CLEANUP ***
    stop = 1;
}

#if 0
https://man7.org/linux/man-pages/man7/epoll.7.html
https://man7.org/linux/man-pages/man2/epoll_ctl.2.html
#endif


// https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642

// The epoll_create system call returns a file descriptor to the newly created epoll kernel data structure. 

// When the EPOLL_CLOEXEC flag is set, any child process forked by the current process will close the epoll descriptor before it execs, so the child process won’t have access to the epoll instance anymore.

// ***
// Avoid forking, and if you must: close all epoll-registered file descriptors before calling execve. Explicitly deregister affected file descriptors from epoll set before calling dup/dup2/dup3 or close.

// https://idea.popcount.org/2017-03-20-epoll-is-fundamentally-broken-22/

// epoll_ctl(EPOLL_CTL_ADD) doesn't actually register a file descriptor. Instead it registers a tuple of a file descriptor and a pointer to underlying kernel object. Most confusingly the lifetime of an epoll subscription is not tied to the lifetime of a file descriptor. It's tied to the life of the kernel object.

// If the close call removes the last pointer to kernel object and causes the object to be freed, then it will cause epoll subscription cleanup. But if there are more pointers to kernel object, more file descriptors, in any process on the system, then close will not cause the epoll subscription cleanup. It is totally possible to receive events on previously closed file descriptors.

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

static std::string evt_type(struct epoll_event *evt)
{
	std::string typ("");

	if (evt == NULL)
		return (typ);
	
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

/*
	epoll_pwait()

setup
		// use this with wait_mask
		// wait_mask .. will REPLACE the current sigset
		// for the duration of the epoll_pwait() call
    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

execute

// epoll_pwait() allows an application to safely wait until either a file descriptor becomes ready or until a signal is caught.

	sigset_t wait_mask;
    sigemptyset(&wait_mask); // mask for DURING epoll_pwait
// pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);
// ready = epoll_wait(epfd, &events, n, timeout);
// pthread_sigmask(SIG_SETMASK, &origmask, NULL);
	this->ecnt = epoll_pwait(this->epfd, this->evts, EPOLL_MAX_EVT, 1000, &wait_mask);
*/

Epoll::Epoll (char ** & _envp) : epfd(-1), ecnt(0), envp(_envp)
{
	this->epfd = epoll_create1(EPOLL_CLOEXEC);
	if (this->epfd < 0)
		throw (std::runtime_error("Epoll : bad create"));
	signal(SIGINT, sigint_handler);
};

Epoll::~Epoll()
{
	WsLog::_(LVL_DBG, TGT_EPOLL, "(~) Epoll");
	this->cleanup(1);
};

void	Epoll::cleanup(int std_err)
{
	std::set<EpollClient*>::iterator it = this->clients.begin();
	while (it != this->clients.end())
		delete (*it++);
	this->clients.clear();
	
	if (this->epfd != -1)
		close(this->epfd);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	if (std_err)
		close(STDERR_FILENO);
}


int	Epoll::add(EpollClient *cli)
{
	int	err;
	// check : cli->get_evt().data.ptr

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "add cli  : ", cli->typ_str());
	// WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "add fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (this->has_client(cli))
	{
		WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "add cli  : already exists");
	}
	err = epoll_ctl(this->epfd, EPOLL_CTL_ADD, cli->get_fd(), cli->get_evt());
	if (err < 0)
	{
		WsLog::_errno(LVL_ERR, TGT_EPOLL_CTL, "epoll_ctl: add");
		delete (cli);
	}
	else 
	{
		this->clients.insert(cli);
	}
	return (err);
}

int	Epoll::mod(EpollClient *cli)
{
	int	err;
	// check : cli->get_evt().data.ptr

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "mod cli  : ", cli->typ_str());
	// WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "mod fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!this->has_client(cli))
	{
		WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "mod cli  : does not exist");
		// return (this->add(cli));
	}
	err = epoll_ctl(this->epfd, EPOLL_CTL_MOD, cli->get_fd(), cli->get_evt());
	if (err < 0)
	{
		WsLog::_errno(LVL_ERR, TGT_EPOLL_CTL, "epoll_ctl: mod");	
	}
	return (err);
}

int	Epoll::del(EpollClient *cli)
{
	int err;

	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "del cli  : ", cli->typ_str());
	// WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "del fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!has_client(cli))
	{
		WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "del cli  : does not exist");
		return (0);
	}
	err = epoll_ctl(this->epfd, EPOLL_CTL_DEL, cli->get_fd(), NULL);
	if (err < 0)
	{
		WsLog::_errno(LVL_ERR, TGT_EPOLL_CTL, "epoll_ctl: del");
	}
	return (err);
}


int	Epoll::rem(EpollClient *cli)
{
	WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "rem cli  : ", cli->typ_str());
	std::set<EpollClient*>::iterator it = this->clients.find(cli);
	if (it != this->clients.end())
	{
		this->del(cli); // VERY IMPORTANT
		delete (cli);
		this->clients.erase(it);
	}
	else
	{
		WsLog::_(LVL_INFO, TGT_EPOLL_CTL, "rem cli  : does not exist");
	}

	return (0);
}

bool Epoll::has_client(EpollClient *cli)
{
	return (this->clients.find(cli) != this->clients.end());
}

EpollClient	*Epoll::get_epc(void *cli)
{
	EpollClient	*epc;
	if (cli == NULL)
		return (NULL);
	epc = reinterpret_cast<EpollClient*>(cli);
	if (epc == NULL)
		return (NULL);
	// if (!this->has_client(epc))
	// 	return (NULL);
	return (epc);
}

struct epoll_event	*Epoll::get_evt(int idx)
{
	if (idx < 0 || idx >= this->ecnt)
		return (NULL);
	return (this->evts + idx);
}


int	Epoll::exec(void)
{
	// WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "wait ...\n");
	
	this->ecnt = epoll_wait(this->epfd, this->evts, EPOLL_MAX_EVT, this->toms);
	if (this->ecnt < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPOLL, "epoll_wait");
	if (this->ecnt == 0)
		return (this->ecnt);
	WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "\necnt  : ", this->ecnt);
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
			// this->check_timeouts()
		}
        else if (e < 0)
			return (1);
		for (int k=0; k < e; k++) 
        {
			evt = this->get_evt(k);
			if (evt == NULL)
			{
				WsLog::_(LVL_WARN, TGT_EPOLL_EVT, "evt NULL");
				continue;
			}
			epc = this->get_epc(evt->data.ptr);
			if (epc == NULL)
			{
				WsLog::_(LVL_WARN, TGT_EPOLL_EVT, "epc NULL");
				continue;
			}
#if 0 // TESTING
			if (!this->has_client(epc))
			{
				std::cerr << "\nEXPECT SHIT\n\n";
				continue;
			}
#endif

WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt tgt  : ", epc->typ_str());
WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt typ  : ", evt_type(evt));
// WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt fd   : ", epc->get_fd()); // DBG_EPC_FD
		
			// epc->events(evt)
            if (evt->events & EPOLLERR) // with (out) .. shit .. things to write (?)
			{
				this->rem(epc);
				continue;
			}
            if (evt->events & EPOLLIN)
            {
				epc->set_lact();
				err = epc->pollin();
				if (err < 0) // && state (?)
				{
					this->rem(epc);
					continue;
				}
            }
            if (evt->events & EPOLLOUT)
            {
				epc->set_lact();
				err = epc->pollout();
				if (err < 0) // && state (?)
				{
					this->rem(epc);
					continue;
				}
			}	
			// down here : connection reset by peer
            // if (evt->events & EPOLLERR)
			// {
			// 	this->rem(epc);
			// 	continue;
			// }
				// nothing more to read .. BUT may still need to write (cgi) output 
			if (evt->events & EPOLLRDHUP)
			{
				this->rem(epc);
				// epc->mod_evt(EPOLLOUT);
				continue;
			} 
			// cgi : may close with (hup)
            if (evt->events == EPOLLHUP)
			{
				epc->hup();
				this->rem(epc);
				continue;
			}
        }
		// this->check_timeouts()
    }
	return (0);
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


