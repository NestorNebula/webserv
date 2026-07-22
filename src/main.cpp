/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:51:20 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/22 12:29:47 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "WsLog.hpp"
#include <iostream>

static void	printServer(const ServerConfig& server, size_t index)
{
	std::cout << "\n===== SERVER #" << index << " =====" << std::endl;
	std::cout << "   host			: " << server.host << std::endl;
	std::cout << "   port			: " << server.port << std::endl;
	std::cout << "   max_body_size	: " << server.max_body_size << std::endl;
	std::cout << "   root			: " << server.root << std::endl;
	std::cout << "   upload		: " << (server.upload ? "on" : "off") << std::endl;
	std::cout << "   upload_dir		: " << server.upload_dir << std::endl;
	std::cout << "   error_pages	: " << std::endl;
	std::map<std::string, std::string>::const_iterator	it;
	for (it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
		std::cout << "		[" << it->first << "] -> " << it->second << std::endl;

	std::cout << "   routes (" << server.routes.size() << ") :" << std::endl;
	for (size_t i = 0; i < server.routes.size(); ++i)
	{
		const RouteConfig& route = server.routes[i];
		std::cout << "		--- route " << route.path << " ---" << std::endl;
		std::cout << "			index		: ";
		for (size_t j = 0; j < route.index.size(); ++j)
		{
			if (j != 0)
				std::cout << ", ";
			std::cout << route.index[j];
		}
		std::cout << std::endl;
		std::cout << "			root		: " << route.root << std::endl;
		std::cout << "			autoindex	: " << (route.autoindex ? "on" : "off") <<std::endl;
		std::cout << "			upload		: " << (route.upload ? "on" : "off") << std::endl;
		if (route.upload)
			std::cout << "			upload_dir	: " << route.upload_dir << std::endl;
		std::cout << "			max_body_size	: " << route.max_body_size << std::endl;
		std::cout << "			redirect	: " << route.redirect << std::endl;
		std::cout << "			cgi		: " << std::endl;
		std::map<std::string, std::string>::const_iterator	cit;
		for (cit = route.cgi.begin(); cit != route.cgi.end(); ++cit)
			std::cout << "				[" << cit->first << "] -> " << cit->second << std::endl;

		std::cout << "			error_pages	: " << std::endl;
		std::map<std::string, std::string>::const_iterator	it;
		for (it = route.error_pages.begin(); it != route.error_pages.end(); ++it)
			std::cout << "				[" << it->first << "] -> " << it->second << std::endl;

		std::cout << "			methods		: ";
		std::set<HttpMethod>::const_iterator	mit;
		for (mit = route.methods.begin(); mit != route.methods.end(); ++mit)
		{
			if (mit != route.methods.begin())
				std::cout << ", ";
			std::cout << methodToString(*mit);
		}
		std::cout << std::endl;
	}
}

int	main(int argc, char **argv)
{
	WsLog::lvl = LVL_ALL;
	WsLog::tgt = TGT_PARSING;

	if (argc != 2)
	{
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return (1);
	}
	try {
		ConfigParser	parser;

		parser.parseFile(argv[1]);

		const std::vector<ServerConfig>& servers = parser.getServers();
		std::cout << "Parsing OK - " << servers.size() << " server(s) found." << std::endl;

		for (size_t i = 0; i < servers.size(); ++i)
			printServer(servers[i], i);
	} catch (const std::exception& e) {
		WsLog::_(LVL_ERR, TGT_CONFIG, e.what());
		return (1);
	}
	return (0);
}
