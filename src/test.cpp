/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 22:53:43 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"


int main (void)
{
    int     err;
    
    WsLog::lvl = 
        LVL_NONE
        | LVL_ERR 
        | LVL_INFO
        | LVL_DBG
        | LVL_WARN
    ;
    WsLog::tgt = 
        TGT_NONE
        | TGT_EPOLL 
        // | TGT_EPOLL_EVT
        // | TGT_EPOLL_CTL
        
        | TGT_EPC
        // | TGT_EPC_RECV
        // | TGT_EPC_SEND
        
        | TGT_CONN
        | TGT_CONN_RECV
        | TGT_CONN_SEND

        | TGT_CGI
        | TGT_CGI_RECV
        | TGT_CGI_SEND
    ;
    
    Epoll   ep;

    Server *serv[2];
    
    // attn : CATCH each (bind) FAIL ...
    serv[0] = new Server(ep, 8080);
    serv[1] = new Server(ep, 8081);
    
    err = ep.loop();

    // TGT_MAIN
    std::cerr << "exit  : " << err << std::endl;

    // delete (serv[0]);
    // delete (serv[1]);
    
    return (err);
}
