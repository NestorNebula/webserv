/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:34 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/06 15:20:55 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_PIPE_HPP
# define CGI_PIPE_HPP

# include "EpollClient.hpp"

class Connection;

class CgiPipe : public EpollClient
{
private:
	CgiPipe (const CgiPipe & that) : 
		EpollClient(that), conn(that.conn) {};
	CgiPipe & operator = (const CgiPipe & ) 
		{ return (*this); }
public:
	CgiPipe (Epoll &_ep, int _fd, Connection & _conn);
	~CgiPipe();
	
	int		pollin(void);
	int		pollout(void);

private:
	Connection	&conn;
};



#endif