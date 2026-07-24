/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 10:21:48 by kdonlon          ###   ########.fr       */
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
	
	WsLog::_(LVL_ERR, TGT_EPOLL, "\n\n\n\n");
	WsLog::_(LVL_ERR, TGT_EPOLL, "SIGINT");

    stop = 1;
}
static void sigpipe_handler(int signo)
{
    (void)signo;
	
	WsLog::_(LVL_ERR, TGT_EPOLL, "\n\n\n\n");
	WsLog::_(LVL_ERR, TGT_EPOLL, "SIGPIPE");
}

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

Epoll::Epoll (char ** & _envp) : epfd(-1), ecnt(0), envp(_envp)
{
	this->epfd = epoll_create1(EPOLL_CLOEXEC);
	if (this->epfd < 0)
		throw (std::runtime_error("Epoll : bad create"));
	signal(SIGINT, sigint_handler);
	signal(SIGPIPE, sigpipe_handler);
};

Epoll::~Epoll()
{
	WsLog::_(LVL_DBG, TGT_EPOLL, "(~) Epoll");
	this->cleanup();
};

void	Epoll::cleanup()
{
	std::set<EpollClient*>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		// this->del(*it); // necessary (?)
		delete (*it++);
	}
	this->clients.clear();
	
	if (this->epfd != -1)
	{
		close(this->epfd);
		this->epfd = -1;
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}


int	Epoll::add(EpollClient *cli)
{
	if (cli->get_evt()->data.ptr == NULL)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "add cli  : bad data ptr");
		return (-1);
	}
	
	int	err;

	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "add cli  : ", cli->typ_str());
	// WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "add fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (this->has_client(cli))
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "add cli  : already exists");
		// return (this->mod(cli));
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
	if (cli->get_evt()->data.ptr == NULL)
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "mod cli  : bad data ptr");
		return (-1);
	}
	
	int	err;

	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod cli  : ", cli->typ_str());
	// WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod evt  : ", evt_type(cli->get_evt()));
	// WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "mod fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!this->has_client(cli))
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "mod cli  : does not exist");
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

	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "del cli  : ", cli->typ_str());
	// WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "del fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!has_client(cli))
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "del cli  : does not exist");
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
	WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "rem cli  : ", cli->typ_str());
	std::set<EpollClient*>::iterator it = this->clients.find(cli);
	if (it != this->clients.end())
	{
		this->del(cli); // VERY IMPORTANT
		delete (cli);
		this->clients.erase(it);
	}
	else
	{
		WsLog::_(LVL_ERR, TGT_EPOLL_CTL, "rem cli  : does not exist");
	}
	// WsLog::_(LVL_DBG, TGT_EPOLL_CTL, "clients  : ", this->clients.size());

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
	this->ecnt = epoll_wait(this->epfd, this->evts, EPOLL_MAX_EVT, this->toms);
	if (this->ecnt < 0)
		return WsLog::_errno(LVL_ERR, TGT_EPOLL, "epoll_wait");
	if (this->ecnt == 0)
		return (this->ecnt);
	WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "\necnt  : ", this->ecnt);
	return (this->ecnt);
}


void	Epoll::check_timeo(void)
{
	time_t	n;
	
	n = time(&n);
	
	std::set<EpollClient*>::iterator it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it)->timeo(n))
		{
			WsLog::_(LVL_ERR, TGT_EPC, "TIMEOUT  : ", (*it)->typ_str());
		}
		it++;
	}
}

int	Epoll::loop(void)
{
	int					e;
	struct epoll_event	*evt;
	EpollClient 		*epc;
	
    while (!stop)
    {
        e = this->exec();
        if (e < 0)
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
			WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt tgt  : ", epc->typ_str());
			// WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt fd   : ", epc->get_fd()); // DBG_EPC_FD
			WsLog::_(LVL_DBG, TGT_EPOLL_EVT, "evt typ  : ", evt_type(evt));
			if (epc->event(evt) < 0)
			{
				this->rem(epc);
			}
        }
		this->check_timeo();
    }
	return (0);
}

