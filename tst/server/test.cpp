/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/22 11:36:34 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"

int main (int argc, char ** argv, char **envp)
{
    std::cerr << "pwd : " << getenv("PWD") << std::endl;
    std::cerr << "argv[0] : " << argv[0] << std::endl;
    
    WsLog::kd();
    if (argc > 1 && argv[1][0] == '0')
        WsLog::tgt = TGT_NONE;
    
    int     err = 0;
    Epoll   *ep = NULL;
    
    try
    {
        ep = new Epoll(envp);
        
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
