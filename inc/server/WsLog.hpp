/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 11:54:32 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WS_LOG_HPP
# define WS_LOG_HPP

# include <unistd.h>
# include <iostream>
# include <sstream>
# include <string>
# include <cstring>
# include <cerrno>

typedef long unsigned int log_lvl;
typedef long unsigned int log_tgt;

# define LVL_NONE	(0)
# define LVL_INFO  	(1UL << 0)
# define LVL_DBG	(1UL << 1)
# define LVL_ERR	(1UL << 2)
# define LVL_WARN	(1UL << 3)
# define LVL_MAX	(1UL << 4)
# define LVL_ALL	(LVL_MAX - 1)


# define TGT_NONE   	(0)
# define TGT_EPOLL_EVT  (1UL << 1)
# define TGT_EPOLL_CTL  (1UL << 2)
# define TGT_EPOLL		(TGT_EPOLL_EVT | TGT_EPOLL_CTL)

# define TGT_EPC_RECV	(1UL << 3)
# define TGT_EPC_SEND	(1UL << 4)
# define TGT_EPC		(TGT_EPC_RECV | TGT_EPC_SEND)

# define TGT_CONN_RECV	(1UL << 5)
# define TGT_CONN_SEND	(1UL << 6)
# define TGT_CONN_DATA	(1UL << 7)
# define TGT_CONN		(TGT_CONN_RECV | TGT_CONN_SEND | TGT_CONN_DATA)

# define TGT_CGI_RECV	(1UL << 8)
# define TGT_CGI_SEND	(1UL << 9)
# define TGT_CGI_DATA	(1UL << 10)
# define TGT_CGI_HEAD	(1UL << 11)
# define TGT_CGI		(TGT_CGI_RECV | TGT_CGI_SEND | TGT_CGI_DATA | TGT_CGI_HEAD)
# define TGT_CGI_ENV	(1UL << 12)

# define TGT_RSRC		(1UL << 13)
# define TGT_RSRC_INFO	(1UL << 14)
# define TGT_RSRC_WAIT	(1UL << 15)

# define TGT_SERV		(1UL << 16)
# define TGT_MAIN		(1UL << 17)

# define TGT_HEAD		(1UL << 18)
# define TGT_BODY		(1UL << 19)


# define TGT_MAX		(1UL << 63)
# define TGT_ALL		(TGT_MAX - 1)


class WsLog
{
private:
	WsLog (void) {}

public:
	static log_lvl  lvl;
	static log_tgt  tgt; 
	
	static void _(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg);
	static void _(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t n);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t i, ssize_t j);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, ssize_t n);

	static int	_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg);

	static void	kd(void);
	static void mm(void) {}
	static void	nh(void) {}
private:
	static bool nolog(log_lvl msg_lvl, log_tgt msg_tgt);
};

#endif