/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/04 15:37:02 by nhoussie         ###   ########.fr       */
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
# define LVL_INFO  	(1 << 0)
# define LVL_DBG	(1 << 1)
# define LVL_ERR	(1 << 2)
# define LVL_WARN	(1 << 3)
# define LVL_MAX	(1 << 4)
# define LVL_ALL	(LVL_MAX - 1)

# define TGT_NONE   	(0)
# define TGT_EPOLL_EVT  (1L << 1)
# define TGT_EPOLL_CTL  (1L << 2)
# define TGT_EPOLL		(TGT_EPOLL_EVT | TGT_EPOLL_CTL)

# define TGT_EPC_RECV	(1L << 3)
# define TGT_EPC_SEND	(1L << 4)
# define TGT_EPC		(TGT_EPC_RECV | TGT_EPC_SEND)

# define TGT_CONN_RECV	(1L << 5)
# define TGT_CONN_SEND	(1L << 6)
# define TGT_CONN_DATA	(1L << 7)
# define TGT_CONN		(TGT_CONN_RECV | TGT_CONN_SEND | TGT_CONN_DATA)

# define TGT_CGI_RECV	(1L << 8)
# define TGT_CGI_SEND	(1L << 9)
# define TGT_CGI_DATA	(1L << 10)
# define TGT_CGI		(TGT_CGI_RECV | TGT_CGI_SEND | TGT_CGI_DATA)

# define TGT_SERV		(1L << 11)
# define TGT_MAIN		(1L << 12)

# define TGT_HEAD		(1L << 13)

// HTTP TGTs using 1L << 21 to 1L << 40
# define TGT_REQ		(1L << 21)

# define TGT_STAT_RES	(1L << 22)
# define TGT_DIR_RES	(1L << 23)
# define TGT_RES		(TGT_STAT_RES | TGT_DIR_RES)

# define TGT_RESP		(1L << 24)

# define TGT_TMP_STRM	(1L << 25)
# define TGT_STRM		(TGT_TMP_STRM | (1L << 26))

# define TGT_SESS_WR	(1L << 27)
# define TGT_SESS_RD	(1L << 28)
# define TGT_SESS		(TGT_SESS_WR | TGT_SESS_RD)

# define TGT_HTTP		(TGT_REQ | TGT_RES | TGT_RESP | TGT_STRM | TGT_SESS)


# define TGT_MAX		(1L << 63)
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
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, ssize_t n);

	static int	_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg);
private:
	static bool nolog(log_lvl msg_lvl, log_tgt msg_tgt);
};

#endif
