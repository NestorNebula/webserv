/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseBlocks.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 14:41:11 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/30 10:36:52 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <sstream>

void	ConfigParser::parseRoute(ServerConfig& current_server)
{
	RouteConfig	route;
	std::map<std::string, std::string>	directives;
	std::set<std::string>				seen_directives;

	consume();
	Token	pathToken = expect(TOKEN_WORD);
	route.path = pathToken.value;
	expect(TOKEN_LBRACE);
	skipNewlines();
	while (peek().type != TOKEN_RBRACE && peek().type != TOKEN_EOF)
	{
		parseDirective(directives, seen_directives);
		skipNewlines();
	}
	expect(TOKEN_RBRACE);

	route.root = directives.count("root")
		? directives["root"] : current_server.root;
	route.upload = directives.count("upload")
		? (directives["upload"] == "on") : current_server.upload;
	if (directives.count("autoindex"))
		route.autoindex = (directives["autoindex"] == "on");

	if (directives.count("methods"))
	{
		std::string			value = directives["methods"];
		std::stringstream	ss(value);
		std::string			token;

		while (std::getline(ss, token, ','))
		{
			size_t	start = token.find_first_not_of(" \t"); // Trim
			size_t	end = token.find_last_not_of(" \t");

			if (start == std::string::npos)
				continue;
			std::string	clean = token.substr(start, end - start + 1);

			HttpMethod	method = stringToMethod(clean);
			if (method == METHOD_UNKNOWN)
				throw ConfigException("Invalid HTTP Method: " + clean);
			route.methods.insert(method);
		}
	} else {
		route.methods = current_server.methods; // Legacy
	}
	current_server.routes.push_back(route);
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
