/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 20:10:50 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"
#include "ConfigParser.hpp"
#include "FilePath.hpp"

#include <deque>

int main (int argc, char ** argv, char **envp)
{   
    WsLog::kd();
    // WsLog::nh();
    // WsLog::mm();

    if (argc < 2)
    {
        std::cerr << "usage: webserv <config>\n";
        return 0;
    }
    if (argc > 2)
    {
        switch(argv[2][0])
        {
        case '0':
           WsLog::lvl = LVL_MAIN;
           WsLog::tgt = TGT_ALL;
           break;
        case 'k':
           WsLog::tgt = TGT_KD;
           break;
        case 'a':
           WsLog::tgt = TGT_ALL; //  & ~(TGT_CGI_HEAD | TGT_CGI_DATA);
           break;
        }
    }
// #kd - conf_file_root
    std::string conf_root;
    if (env_pwd(envp, conf_root))
    {
        WSLOG(LVL_ERR, TGT_MAIN, "couldn't detect working directory");
        return (0);
    }
    if (!setWorkingDirectory(argv[1], conf_root)) 
    {
        WSLOG(LVL_ERR, TGT_MAIN, "couldn't setup working directory");
        return 0;
    }
    
// #kd - conf_file_root
    ConfigParser parser(conf_root);
    try 
    {
        parser.parseFile(getConfigFileName(argv[1]));
    } 
    catch (std::exception &e) 
    {
        WSLOG(LVL_ERR, TGT_MAIN, "ex: main\n", e.what());
        return 0;
    }

    const std::vector<ServerConfig> &servers = parser.getServers();

    int     err = 0;
    Epoll   *ep = NULL;
    
    try
    {
        ep = new Epoll(envp);
       
        err = ep->serve(servers);
        if (err)
          err = ep->loop();
        WSLOG(LVL_ERR, TGT_MAIN, "exit: ", err);
    }
    catch(const std::exception& e)
    {
        WSLOG(LVL_ERR, TGT_MAIN, "ex: main\n", e.what());
    }

    delete (ep);
    return (err);
}
