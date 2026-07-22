/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsLog.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 11:56:36 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/22 10:35:02 by kdonlon          ###   ########.fr       */
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
    "env   : ",
    "serv  : ",
    "main  : ",
    "head  : ",
    "body  : ",
    "rsrc  : "
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
    if (tgt & TGT_CGI_ENV)
        return (tgt_str[5]);
    if (tgt & TGT_SERV)
        return (tgt_str[6]);
    if (tgt & TGT_MAIN)
        return (tgt_str[7]);
    if (tgt & TGT_HEAD)
        return (tgt_str[8]);
    if (tgt & TGT_BODY)
        return (tgt_str[9]);
    if (tgt & (TGT_RSRC | TGT_RSRC_INFO | TGT_RSRC_WAIT))
        return (tgt_str[10]);

    return (tgt_str[0]);
}

bool    WsLog::nolog(log_lvl msg_lvl, log_tgt msg_tgt)
{
    if ((msg_lvl & WsLog::lvl) == LVL_ERR)
        return (false);
    // if ((msg_lvl & WsLog::lvl) == LVL_INFO)
    //     return (false);
    if ((msg_lvl & WsLog::lvl) && (msg_tgt & WsLog::tgt))
        return (false);
    return (true);
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << std::endl;
    std::cerr << stream.str();
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream<< tgt_prefix(msg_tgt) << msg << "[" << n << "]" << std::endl;
    std::cerr << stream.str();
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, ssize_t i, ssize_t j)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream<< tgt_prefix(msg_tgt) << msg << "[" << i << " / " << j << "]" << std::endl;
    std::cerr << stream.str();
}


void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg, std::string str)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << str << std::endl;
    std::cerr << stream.str();
}

void    WsLog::_(log_lvl msg_lvl, log_tgt msg_tgt, ssize_t n)
{
    if (WsLog::nolog(msg_lvl, msg_tgt))
        return;

    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << n << std::endl;
    std::cerr << stream.str();
}


int	WsLog::_errno(log_lvl msg_lvl, log_tgt msg_tgt, std::string msg)
{
    (void) msg_lvl;
    
    std::stringstream stream;
    stream << tgt_prefix(msg_tgt) << msg << std::endl;
    stream << "error : " << strerror(errno) << std::endl;
    std::cerr << stream.str();

    return (-1);
}


void    WsLog::kd(void)
{
    WsLog::lvl = LVL_NONE
        | LVL_ERR 
        | LVL_WARN
        | LVL_INFO
        | LVL_DBG
    ;
    WsLog::tgt = TGT_NONE
        // | TGT_EPOLL 
        | TGT_EPOLL_EVT
        // | TGT_EPOLL_CTL
        
        // | TGT_EPC
        // | TGT_EPC_RECV
        // | TGT_EPC_SEND
        
        
        | TGT_CONN
        | TGT_CONN_RECV
        | TGT_CONN_SEND
        | TGT_CONN_DATA

        // | TGT_CGI - dangerous data 
        | TGT_CGI_RECV
        | TGT_CGI_SEND
        | TGT_CGI_DATA

        // | TGT_SERV
        // | TGT_MAIN

        | TGT_HEAD
        // | TGT_BODY
        | TGT_RSRC
        | TGT_RSRC_INFO
        // | TGT_RSRC_WAIT
    ;
    
    // WsLog::tgt = TGT_NONE;

    // WsLog::lvl = LVL_INFO;
    // WsLog::tgt = TGT_ALL;
}