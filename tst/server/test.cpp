/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/06 20:49:14 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"

int main (int argc, char ** argv, char **envp)
{
    // std::cerr << "pwd : " << getenv("PWD") << std::endl;
    // std::cerr << "argv[0] : " << argv[0] << std::endl;
    
    WsLog::kd();
    if (argc > 1)
    {
        switch(argv[1][0])
        {
        case '0':
           WsLog::tgt = TGT_NONE;
           break;
        case 'a':
           WsLog::tgt = TGT_ALL; //  & ~(TGT_CGI_HEAD | TGT_CGI_DATA);
           break;
        }
    }
    int     err = 0;
    Epoll   *ep = NULL;
    
    try
    {
        ep = new Epoll(envp);
        
// ATTN : catch failed Server construction individually (!)
        new Server(ep, 8080);
        new Server(ep, 8081);
        new Server(ep, 8082);
        
        err = ep->loop();
        WsLog::_(LVL_INFO, TGT_MAIN, "exit: ", err);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    delete (ep);
    return (err);
}
