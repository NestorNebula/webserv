/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:31 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/27 08:18:47 by kdonlon          ###   ########.fr       */
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

# ifndef NOLOG
#  define NOLOG 0
# endif

# if NOLOG
#  define WSLOG(...)
#  define WSCOL(...) 
#else
#  define WSLOG WsLog::_
#  define WSCOL WsLog::color
#endif


typedef long unsigned int log_lvl;
typedef long unsigned int log_tgt;

# define LVL_NONE	(0)
# define LVL_MAIN   (1UL << 0)
# define LVL_INFO  	(1UL << 1)
# define LVL_DBG	(1UL << 2)
# define LVL_ERR	(1UL << 3)
# define LVL_WARN	(1UL << 4)
# define LVL_TMP	(1UL << 5)
# define LVL_MAX	(1UL << 6)
# define LVL_ALL	(LVL_MAX - 1)


# define TGT_NONE   	(0)
# define TGT_EPOLL_CNT	(1UL << 1)
# define TGT_EPOLL_EVT  (1UL << 2)
# define TGT_EPOLL_CTL  (1UL << 3)
# define TGT_EPOLL		(TGT_EPOLL_CNT | TGT_EPOLL_EVT | TGT_EPOLL_CTL)

# define TGT_EPC_RECV	(1UL << 4)
# define TGT_EPC_SEND	(1UL << 5)
# define TGT_EPC		(TGT_EPC_RECV | TGT_EPC_SEND)

# define TGT_CONN_RECV	(1UL << 6)
# define TGT_CONN_SEND	(1UL << 7)
# define TGT_CONN_DATA	(1UL << 8)
# define TGT_CONN		(TGT_CONN_RECV | TGT_CONN_SEND)

# define TGT_CGI_RECV	(1UL << 9)
# define TGT_CGI_SEND	(1UL << 10)
# define TGT_CGI_DATA	(1UL << 11)
# define TGT_CGI_HEAD	(1UL << 12)
# define TGT_CGI		(TGT_CGI_RECV | TGT_CGI_SEND)
# define TGT_CGI_ENV	(1UL << 13)
# define TGT_CGI_ERR	(1UL << 14)

# define TGT_RSRC		(1UL << 15)
# define TGT_RSRC_INFO	(1UL << 16)
# define TGT_RSRC_WAIT	(1UL << 17)
# define TGT_RSRC_STAT	(1UL << 18)

# define TGT_SERV		(1UL << 19)
# define TGT_MAIN		(1UL << 20)

// ATTN : poaching .. (sorry, noah)
# define TGT_HEAD		(1UL << 21)
# define TGT_BODY		(1UL << 22)
# define TGT_FCGI		(1UL << 23)
# define TGT_FCGI_PARSE (1UL << 24)

# define TGT_SERV_ALL	((1UL << 25) - 1)




// HTTP TGTs using 1UL << 31 to 1UL << 40
# define TGT_REQ		(1UL << 31)

# define TGT_STAT_RES	(1UL << 32)
# define TGT_DIR_RES	(1UL << 33)
# define TGT_BUI_RES	(1UL << 34)
# define TGT_RES		(TGT_STAT_RES | TGT_DIR_RES | TGT_BUI_RES)

# define TGT_RESP		(1UL << 35)

# define TGT_TMP_STRM	(1UL << 36)
# define TGT_STRM		(TGT_TMP_STRM | (1UL << 37))

# define TGT_SESS_WR	(1UL << 38)
# define TGT_SESS_RD	(1UL << 39)
# define TGT_SESS		(TGT_SESS_WR | TGT_SESS_RD | (1UL << 40))

# define TGT_HTTP		(TGT_REQ | TGT_RES | TGT_RESP | TGT_STRM | TGT_SESS)

// PARSING TGTs using 1UL << 41 to 1UL << 60
# define TGT_CONFIG     (1UL << 41)
# define TGT_PARSER     (1UL << 42)
# define TGT_LEXER      (1UL << 43)
# define TGT_VALIDATE   (1UL << 44)

# define TGT_PARSE_ALL  (TGT_CONFIG | TGT_PARSER | TGT_LEXER | TGT_VALIDATE)


# define TGT_MAX		(1UL << 63)
# define TGT_ALL		(TGT_MAX - 1UL)

enum
{
	WSL_RED = 1,
	WSL_GREEN,
	WSL_YELLOW,
	WSL_BLUE,
	WSL_PURPLE,
	WSL_CYAN
};

class WsLog
{
private:
	WsLog (void) {}

public:
	static log_lvl  	lvl;
	static log_tgt  	tgt; 
	static std::string	col;

	static void color(int c);
	static void _(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg);
	static void _(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t n);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t i, ssize_t j);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str);
	static void	_(log_lvl msg_lvl, log_tgt msg_tgt, ssize_t n);

	static int	_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg);

	static void	kd(void);
	static void mm(void) {}
	static void	nh(void);
private:
	static bool nolog(log_lvl msg_lvl, log_tgt msg_tgt);
	static void op(std::stringstream & stream);
};

#endif
