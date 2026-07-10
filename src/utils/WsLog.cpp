/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:36 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/10 10:34:16 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

log_lvl WsLog::lvl = LVL_NONE;
log_tgt WsLog::tgt = TGT_NONE;

static const std::string tgt_str[] =
{
    "",
    "epoll : ",
    "epc   : ",
    "conn  : ",
    "cgi   : ",
    "serv  : ",
    "main  : ",
	"cfg   : "
};

static const std::string &tgt_prefix(log_tgt tgt)
{
    if (tgt & TGT_EPOLL)
        return (tgt_str[1]);
    if (tgt & TGT_EPC)
        return (tgt_str[2]);
    if (tgt & TGT_CONN)
        return (tgt_str[3]);
    if (tgt & TGT_CGI)
        return (tgt_str[4]);
    if (tgt & TGT_SERV)
        return (tgt_str[5]);
    if (tgt & TGT_MAIN)
        return (tgt_str[6]);
	if (tgt & TGT_PARSING)
		return (tgt_str[7]);

    return (tgt_str[0]);
}


bool    WsLog::nolog(log_lvl msg_lvl, log_tgt msg_tgt)
{
    if ((msg_lvl & WsLog::lvl) == LVL_ERR)
        return (false);
    if ((msg_lvl & WsLog::lvl) && (msg_tgt & WsLog::tgt))
        return (false);
    return (true);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::cerr << tgt_prefix(msg_tgt) << msg << std::endl;
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, int n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::cerr << tgt_prefix(msg_tgt) << msg << "[" << n << "]" << std::endl;
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::cerr << tgt_prefix(msg_tgt) << msg << str << std::endl;
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, int n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::cerr << tgt_prefix(msg_tgt) << n << std::endl;
}

int	WsLog::_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    (void) msg_lvl;
    std::cerr << tgt_prefix(msg_tgt) << msg << std::endl;
    std::cerr << "error : " << strerror(errno) << std::endl;
    return (-1);
}
