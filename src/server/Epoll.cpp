/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Epoll.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 19:19:57 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/29 21:30:08 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <vector>

#include "Epoll.hpp"
#include "EpollClient.hpp"

#include "Server.hpp"
#include "Connection.hpp"

volatile sig_atomic_t stop = 0;

static void sigint_handler(int signo)
{
    (void)signo;
	
	WSLOG(LVL_ERR, TGT_EPOLL, "\n\n\n\n");
	WSLOG(LVL_ERR, TGT_EPOLL, "SIGINT");

    stop = 1;
}
static void sigpipe_handler(int signo)
{
    (void)signo;
	
	WSLOG(LVL_ERR, TGT_EPOLL, "\n\n\n\n");
	WSLOG(LVL_ERR, TGT_EPOLL, "SIGPIPE");
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

std::string evt_type(int evt)
{
	std::string typ("");

	if (evt < 0)
	{
		typ += "(-) ";
		evt = -evt;
	}
	
	if (evt & EPOLLIN)
		typ += (evt_name[0]);
	if (evt & EPOLLOUT)
		typ += (evt_name[1]);
	if (evt & EPOLLRDHUP)
		typ += (evt_name[2]);
	if (evt & EPOLLPRI)
		typ += (evt_name[3]);
	if (evt & EPOLLERR)
		typ += (evt_name[4]);
	if (evt & EPOLLHUP)
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
	WSLOG(LVL_DBG, TGT_EPOLL, " (~) Epoll");
	this->cleanup();
};

void	Epoll::cleanup()
{
	std::set<EpollClient*>::iterator it;

	const int cli_typ[] = {EPC_CGI, EPC_FCGI, EPC_CONN, EPC_SERV, -1};
	const int	*typ = cli_typ;
	
	while (*typ != -1)
	{
		it = this->clients.begin();
		while (it != this->clients.end())
		{
			if ( (*it)->get_typ() == *typ)
			{
				try 
				{	
					delete (*it);
				}
				catch(const std::exception& e)
				{
					WSLOG(LVL_DBG, TGT_EPOLL, " (~) EpollClient\n", e.what());
				}		
				this->clients.erase(it++);
			}
			else
			{
				++it;
			}
		}
		typ++;
	}
	it = this->clients.begin();
	std::set<EpollClient*>::iterator it;

	const int cli_typ[] = {EPC_CGI, EPC_FCGI, EPC_CONN, EPC_SERV, -1};
	const int	*typ = cli_typ;
	
	while (*typ != -1)
	{
		it = this->clients.begin();
		while (it != this->clients.end())
		{
			if ( (*it)->get_typ() == *typ)
			{
				try 
				{	
					delete (*it);
				}
				catch(const std::exception& e)
				{
					WSLOG(LVL_DBG, TGT_EPOLL, " (~) EpollClient\n", e.what());
				}		
				this->clients.erase(it++);
			}
			else
			{
				++it;
			}
		}
		typ++;
	}
	it = this->clients.begin();
	while (it != this->clients.end())
	{
		try 
		{	
			delete (*it);
			delete (*it);
		}
		catch(const std::exception& e)
		{
			WSLOG(LVL_DBG, TGT_EPOLL, " (~) EpollClient\n", e.what());
		}
		this->clients.erase(it++);
		this->clients.erase(it++);
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
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli add  : bad data ptr");
		return (-1);
	}
	
	int	err;

	WSLOG(LVL_DBG, TGT_EPOLL_CTL, "cli add  : ", cli->typ_str());
	// WSLOG(LVL_DBG, TGT_EPOLL_CTL, "add fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (this->has_client(cli))
	{
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli add  : already exists");
		return (this->mod(cli));
		return (0);
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
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli mod  : bad data ptr");
		return (-1);
	}
	
	int	err;

	WSLOG(LVL_DBG, TGT_EPOLL_CTL, "cli mod  : ", cli->typ_str());
	// WSLOG(LVL_DBG, TGT_EPOLL_CTL, "mod evt  : ", evt_type(cli->get_evt()->events));
	// WSLOG(LVL_DBG, TGT_EPOLL_CTL, "mod fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!this->has_client(cli))
	{
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli mod  : does not exist");
		return (this->add(cli));
	}
	err = epoll_ctl(this->epfd, EPOLL_CTL_MOD, cli->get_fd(), cli->get_evt());
	if (err < 0)
	{
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli mod  : ", cli->get_fd());
		WsLog::_errno(LVL_ERR, TGT_EPOLL_CTL, "epoll_ctl: mod ");	
	}
	return (err);
}

int	Epoll::del(EpollClient *cli)
{
	int err;

	WSLOG(LVL_DBG, TGT_EPOLL_CTL, "cli del  : ", cli->typ_str());
	// WSLOG(LVL_DBG, TGT_EPOLL_CTL, "del fd   : ", cli->get_fd()); // DBG_EPC_FD
	if (!has_client(cli))
	{
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli del  : does not exist");
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
	WSLOG(LVL_DBG, TGT_EPOLL_CTL, "cli rem  : ", cli->typ_str());
	std::set<EpollClient*>::iterator it = this->clients.find(cli);
	if (it != this->clients.end())
	{
		this->del(cli); // VERY IMPORTANT
		delete (cli);
		this->clients.erase(it);
	}
	else
	{
		WSLOG(LVL_ERR, TGT_EPOLL_CTL, "cli rem  : does not exist");
	}
	// WSLOG(LVL_DBG, TGT_EPOLL_CTL, "clients  : ", this->clients.size());
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
	if (!this->has_client(epc))
		return (NULL);
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
		return (WsLog::_errno(LVL_ERR, TGT_EPOLL, "epoll_wait"));
	if (this->ecnt == 0)
		return (this->ecnt);
	WSLOG(LVL_DBG, TGT_EPOLL_CNT, "ecnt  : ", this->ecnt);
	return (this->ecnt);
}


void	Epoll::check_timeo(void)
{
	time_t	n;
	
	n = time(&n);
	
	std::set<EpollClient*>::iterator it;
	
	it = this->clients.begin();
	while (it != this->clients.end())
	{
		if ((*it)->timeo(n))
		{
			// WSLOG(LVL_DBG, TGT_EPC, "TIMEOUT  : ", (*it)->typ_str());
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
				WSLOG(LVL_WARN, TGT_EPOLL_EVT, "evt NULL");
				continue;
			}
			epc = this->get_epc(evt->data.ptr);
			if (epc == NULL)
			{
				WSLOG(LVL_WARN, TGT_EPOLL_EVT, "epc NULL");
				continue;
			}
			
			WSLOG(LVL_DBG, TGT_EPOLL_EVT, "");
			WSLOG(LVL_DBG, TGT_EPOLL_EVT, "evt tgt  : ", epc->typ_str());
			WSLOG(LVL_DBG, TGT_EPOLL_EVT, "evt fd   : ", epc->get_fd()); // DBG_EPC_FD
			WSLOG(LVL_DBG, TGT_EPOLL_EVT, "evt typ  : ", evt_type(evt->events));
			
			try
			{
				if (epc->event(evt) < 0)
					this->rem(epc);
			}
			catch(const std::logic_error& e)
			{
				WSLOG(LVL_DBG, TGT_EPOLL, "ex: loop\n", e.what());
				this->rem(epc);
			}
			catch(const std::exception& e)
			{
				WSLOG(LVL_DBG, TGT_EPOLL, "ex: loop\n", e.what());
				this->rem(epc);
			}
        }
		this->check_timeo();	
    }
	return (0);
}

int	Epoll::serve(const std::vector<ServerConfig> &serv_list)
{
	int	err = 0;
	std::vector<ServerConfig>::const_iterator it = serv_list.begin();
	std::vector<ServerConfig>::const_iterator ite = serv_list.end();
	for ( ;it != ite; it++)
	{
		try
		{
			new Server(this, it->port, *it);
			err = 1;
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			return (0);
		}
	}
	return (err);
}




// It is not currently possible to reliably delete epoll items when using the same epoll set from multiple threads. After calling epoll_ctl with EPOLL_CTL_DEL, another thread might still be executing code related to an event for that epoll item (in response to epoll_wait). Therefore the deleting thread does not know when it is safe to delete resources pertaining to the associated epoll item because another thread might be using those resources. 

// HM : do not delete ..CgiPipe .. until .. we are sure (cgi) is complete (?)
// so .. keep them in the epoll .. but .. disabled

// the reading application would be tied up for a long period; 
// meanwhile, it does not service I/O events on the other file descriptors—those descriptors are starved of service by the application. 

// The solution to file descriptor starvation is for the application to maintain a user-space data structure that caches the readiness of each of the file descriptors that it is monitoring. 

// so .. cache as (READY) .. until .. recv/send ZERO ...

// EPOLLONESHOT. If this flag is specified in the events mask for a file descriptor, then, once the file descriptor becomes ready and is returned by a call to epoll_wait(), it is disabled from further monitoring (but remains in the interest list). If the application is interested in monitoring file descriptor once more, then it must re-enable the file descriptor using the epoll_ctl(EPOLL_CTL_MOD) operation. 

// A defunct or "zombie" process in Linux occurs when a child process finishes running via fork(), but its parent process does not read its exit status using a wait() system call. The process remains in the table holding its PID until reaped.


