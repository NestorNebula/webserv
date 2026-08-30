/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:51:20 by mamarti           #+#    #+#             */
/*   Updated: 2026/08/30 14:48:06 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/parsing/ConfigParser.hpp"
#include "utils/WsLog.hpp"
#include <iostream>

static void printServer(const ServerConfig& server, size_t index)
{
    std::cout << "\n===== SERVER #" << index << " =====" << std::endl;
    std::cout << "   host           : " << server.host << std::endl;
    std::cout << "   port           : " << server.port << std::endl;
    std::cout << "   max_body_size  : " << server.max_body_size << std::endl;
    std::cout << "   root           : " << server.root << std::endl;
    std::cout << "   upload         : " << (server.upload ? "on" : "off") << std::endl;
    std::cout << "   upload_dir     : " << server.upload_dir << std::endl;

    // Nouvelles variables kdonlon
    if (!server.fcgi_sock.empty())
        std::cout << "   fcgi_sock      : " << server.fcgi_sock << std::endl;
    if (!server.pycgi_dir.empty())
        std::cout << "   pycgi_dir      : " << server.pycgi_dir << std::endl;

    std::cout << "   error_pages    : " << std::endl;
    std::map<std::string, std::string>::const_iterator  it;
    for (it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
        std::cout << "      [" << it->first << "] -> " << it->second << std::endl;

    std::cout << "   routes (" << server.routes.size() << ") :" << std::endl;
    for (size_t i = 0; i < server.routes.size(); ++i)
    {
        const RouteConfig& route = server.routes[i];
        std::cout << "      --- route " << route.path << " ---" << std::endl;
        std::cout << "          index       : ";
        for (size_t j = 0; j < route.index.size(); ++j)
        {
            if (j != 0)
                std::cout << ", ";
            std::cout << route.index[j];
        }
        std::cout << std::endl;
        std::cout << "          root        : " << route.root << std::endl;
        std::cout << "          autoindex   : " << (route.autoindex ? "on" : "off") <<std::endl;
        std::cout << "          upload      : " << (route.upload ? "on" : "off") << std::endl;
        if (route.upload)
            std::cout << "          upload_dir  : " << route.upload_dir << std::endl;
        std::cout << "          max_body_size   : " << route.max_body_size << std::endl;
        std::cout << "          redirect    : " << route.redirect << std::endl;
        std::cout << "          cgi     : " << std::endl;
        std::map<std::string, std::string>::const_iterator  cit;
        for (cit = route.cgi.begin(); cit != route.cgi.end(); ++cit)
            std::cout << "              [" << cit->first << "] -> " << cit->second << std::endl;

        std::cout << "          error_pages : " << std::endl;
        std::map<std::string, std::string>::const_iterator  err_it;
        for (err_it = route.error_pages.begin(); err_it != route.error_pages.end(); ++err_it)
            std::cout << "              [" << err_it->first << "] -> " << err_it->second << std::endl;

        std::cout << "          methods     : ";
        std::set<HttpMethod>::const_iterator    mit;
        for (mit = route.methods.begin(); mit != route.methods.end(); ++mit)
        {
            if (mit != route.methods.begin())
                std::cout << ", ";
            std::cout << methodToString(*mit);
        }
        std::cout << std::endl;
    }
}

int main(int argc, char **argv)
{
    WsLog::lvl = LVL_ALL;
    WsLog::tgt = TGT_PARSE_ALL;

    if (argc != 2)
    {
        std::cerr << "Usage: ./test_parser <config_file>" << std::endl;
        return (1);
    }
    try {
        // Ajout du paramètre obligatoire pour le constructeur
        std::string conf_root = "./";
        ConfigParser parser(conf_root);

        parser.parseFile(argv[1]);

        const std::vector<ServerConfig>& servers = parser.getServers();
        std::cout << "Parsing OK - " << servers.size() << " server(s) found." << std::endl;

        for (size_t i = 0; i < servers.size(); ++i)
            printServer(servers[i], i);
    } catch (const std::exception& e) {
        WsLog::_(LVL_ERR, TGT_CONFIG, e.what());
        std::cerr << "\033[0;91m" << e.what() << "\033[0;39m" << std::endl;
        return (1);
    }
    return (0);
}
