/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseBlocks.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 14:41:11 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/29 16:17:30 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/ConfigParser.hpp"

void	ConfigParser::parseRoute(ServerConfig& current_server)
{
	(void) current_server;
}

void	ConfigParser::parseDirective(std::map<std::string, std::string>& directives_map,
			std::set<std::string>& seen_directives)
{
	(void) directives_map;
	(void) seen_directives;
}

void	ConfigParser::parseServer()
{
	ServerConfig	server;

	std::map<std::string, std::string>	directives;
	std::set<std::string>				seen_directives;

	expect(TOKEN_LBRACE);
	skipNewlines();
	while (peek().type != TOKEN_RBRACE && peek().type != TOKEN_EOF)
	{
		if (peek().type == TOKEN_WORD && peek().value == "route")
			parseRoute(server);
		else
			parseDirective(directives, seen_directives);
		skipNewlines();
	}
	expect(TOKEN_RBRACE);

	if (directives.count("listen"))
	{
		std::string	value = directives["listen"];
		size_t		colon_pos = value.find(':');

		if (colon_pos != std::string::npos)
		{
			server.host = value.substr(0, colon_pos);
			server.port = std::atoi(value.substr(colon_pos + 1).c_str());
		} else {
			server.host = "127.0.0.1";
			server.port = std::atoi(value.c_str());
		}
	}
	if (directives.count("max_body_size"))
		server.max_body_size = parseSize(directives["max_body_size"]);
	if (directives.count("root"))
		server.root = directives["root"];
	if (directives.count("upload") && directives["upload"] == "on")
		server.upload = true;
	if (directives.count("upload_dir"))
		server.upload_dir = directives["upload_dir"];

	// Extraction of error pages
	for (std::map<std::string, std::string>::iterator it = directives.begin();
		it != directives.end(); ++it)
	{
		if (it->first.find("error_page ") == 0)
		{
			std::string code = it->first.substr(11);
			server.error_pages[code] = it->second;
		}
	}
	validateServerConfig(server);
	_servers.push_back(server);
}
