/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiPipe.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 19:27:34 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/02 11:46:44 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_PIPE_HPP
# define CGI_PIPE_HPP

# include "EpollClient.hpp"

class Connection;

class CgiPipe : public EpollClient
{
private:
	CgiPipe (const CgiPipe & that) : EpollClient(EPC_CGI, that.fd), conn(that.conn) {};
	CgiPipe & operator = (const CgiPipe & that) { return (*this); }
public:
	CgiPipe (int _fd, Connection & _conn);
	~CgiPipe();
	
	int		pollin(void);
	int		pollout(void);

private:
	Connection	&conn;
};



#endif