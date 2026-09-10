/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/10 11:00:39 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"
#include "FilePath.hpp"
#include "FilePath.hpp"
#include "ConfigParser.hpp"
#include "Epoll.hpp"
#include "Epoll.hpp"

#include "WsTime.hpp"
int main (int argc, char ** argv, char **envp)
{
    if (argc < 2)
    {
        std::cerr << "usage: webserv <config>\n";
        return 0;
    }

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
