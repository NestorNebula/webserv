/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/30 15:29:36 by kdonlon          ###   ########.fr       */
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
        | TGT_CONN
        | TGT_EPC
    ;
    
    Epoll   ep;

    Server *serv[2];
    
    // attn : CATCH each (bind) FAIL ...
    serv[0] = new Server(ep, 8080);
    serv[1] = new Server(ep, 8081);
    
    err = ep.loop();

    std::cerr << "exit  : " << err << std::endl;

    // delete (serv[0]);
    // delete (serv[1]);
    
    return (err);
}


// close failed in file object destructor:
// sys.excepthook is missing
// lost sys.stderr
