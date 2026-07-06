/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 21:11:45 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <unistd.h>
# include "Epoll.hpp"
# include "EpollClient.hpp"
# include <iostream>
# include <sys/wait.h>
# include <cstring>
# include <algorithm>

// # include <map>

#include <sstream>

template <typename T>
std::string num_2_str(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

class CgiEnv
{
private:
	CgiEnv (const CgiEnv & that);
	CgiEnv & operator = (const CgiEnv & )
		{ return (*this); }
public:
	CgiEnv(void);
	~CgiEnv();
	
	void		add(const char *key, const char *val);
	void		add(const char *key, int n);
	const char	**gen(void);
	// from_header(<map>)
private:
	std::vector<std::string>	data;
	const char					**res;
};


class Server;


#define CONN_HAS_HEAD 1 
#define CONN_HAS_RSRC 2
#define CONN_SENT_RESP 3
#define RSRC_SENT_BODY 4
#define CONN_SENT_LENGTH 5

class Connection : public EpollClient
{
private:
	Connection (const Connection & that) : 
		EpollClient(that), 
		serv(that.serv), req_cnt(0) {}
	Connection & operator = (const Connection & ) 
		{ return (*this); }
public:
	
	Connection (Epoll &_ep, int _fd, Server &_serv);
	~Connection();
	
	ssize_t		pollin(void);
	ssize_t		pollout(void);
	int			hup(void) { return (0); }
	
private:
// TESTING
	std::string	header(const char *key);
	std::string	head;
private:
	Server		&serv;
	int			exec_cgi(void);
	
	int			req_cnt;
// TESTING
	std::string	resp;
public:
	int			state;
};

#endif

