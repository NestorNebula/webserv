/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/08 00:24:28 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"

int main (int, char **, char **envp)
{
    WsLog::lvl = LVL_NONE
        | LVL_ERR 
        | LVL_WARN
        | LVL_INFO
        | LVL_DBG
    ;
    WsLog::tgt = TGT_NONE
        // | TGT_ALL
        // | TGT_EPOLL 
        | TGT_EPOLL_EVT
        | TGT_EPOLL_CTL
        
        // | TGT_EPC
        // | TGT_EPC_RECV
        // | TGT_EPC_SEND
        
        // | TGT_CONN
        // | TGT_CONN_RECV
        // | TGT_CONN_SEND
        // | TGT_CONN_DATA

        // | TGT_CGI
        // | TGT_CGI_RECV
        // | TGT_CGI_SEND
        // | TGT_CGI_DATA

        // | TGT_SERV
        // | TGT_MAIN
    ;
    
    // WsLog::tgt = TGT_NONE;
    
    Epoll   ep(envp);
    
    new Server(ep, 8080);
    new Server(ep, 8081);
    new Server(ep, 8082);
    
    int err = ep.loop();

    WsLog::_(LVL_INFO, TGT_MAIN, "exit  : ", err);
    return (err);
}
