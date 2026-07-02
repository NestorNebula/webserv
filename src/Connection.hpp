/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:23:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 16:03:33 by kdonlon          ###   ########.fr       */
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
std::string sstr(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}


class CgiEnv
{
public:
	CgiEnv(void) : res(NULL) {}
	~CgiEnv()
	{
		if (res)
			delete[] res;
	}

	void	add(const char *key, const char *val)
	{
		// <map> first .. to override multiple (?)
		data.push_back(std::string(key) + std::string("=") + std::string(val));
	}
	void	add(const char *key, int n)
	{
		data.push_back(std::string(key) + std::string("=") + sstr(n));
	}
	const char	**gen(void)
	{
		if (res)
			delete[] res;
		size_t	cnt	= data.size();

		res = new const char*[cnt + 1];
		const char	**ins = res;
		
		std::vector<std::string>::iterator it = data.begin();
		while (it != data.end())
		{
			*ins++ = it->c_str();
			it++;
		}
		*ins = NULL;
		return (res);
		// create strings key=val
		// malloc the (char**)
	}

private:
	std::vector<std::string>	data;
	const char					**res;
};

class Server;




// #include <string>
// #include <algorithm>
// #include <cctype>

// bool caseInsensitiveCompare(char a, char b)
// {
//     return std::tolower(static_cast<unsigned char>(a)) ==
//            std::tolower(static_cast<unsigned char>(b));
// }

// std::string::size_type findCaseInsensitive(const std::string& str,
//                                            const std::string& sub)
// {
//     std::string::const_iterator it =
//         std::search(str.begin(), str.end(),
//                     sub.begin(), sub.end(),
//                     caseInsensitiveCompare);

//     if (it == str.end())
//         return std::string::npos;

//     return it - str.begin();
// }


class Connection : public EpollClient
{
private:
	Connection (const Connection & that);
	Connection & operator = (const Connection & that) { return (*this); }
public:
	Server		&serv;
	
	Connection (int _fd, Server &_serv);
	~Connection();
	
	int			pollin(void);
	int			pollout(void);
	
	std::string	header(const char *key);
	std::string	head;
private:
	int			exec_cgi(void);
	
	int			req_cnt;
	int			filedes; // "response"
	
};

#endif

