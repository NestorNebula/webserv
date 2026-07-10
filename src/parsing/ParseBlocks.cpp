/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseBlocks.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 14:41:11 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/10 13:00:23 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "ParserUtils.hpp"
#include <sstream>
#include <cstdlib>

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

	if (directives.count("listen"))
		throw	ConfigException("'listen' directive is not allowed in a route block.");
	if (directives.count("root"))
    	route.root = directives["root"];
	if (directives.count("autoindex"))
		route.autoindex = parseOnOff(directives["autoindex"]);
	if (directives.count("upload"))
		route.upload = parseOnOff(directives["upload"]);
	if (directives.count("max_body_size"))
		route.max_body_size = parseSize(directives["max_body_size"]);
	if (directives.count("methods"))
		route.methods = parseMethods(directives["methods"]);
	if (directives.count("index"))
		route.index = splitList(directives["index"]);
	if (directives.count("upload_dir"))
		route.upload_dir = directives["upload_dir"];
	if (directives.count("redirect"))
		route.redirect = directives["redirect"];

	for (std::map<std::string, std::string>::iterator it = directives.begin();
		it != directives.end(); ++it)
	{
		if (it->first.find("error_page ") == 0)
		{
			std::string code = it->first.substr(11);
			route.error_pages[code] = it->second;
		}
		else if (it->first.find("cgi ") == 0)
		{
			std::string	ext = it->first.substr(4); // cgi .php -> .php
			route.cgi[ext] = it->second;
		}
	}
	current_server.routes.push_back(route);
}

void	ConfigParser::parseDirective(std::map<std::string, std::string>& directives_map,
			std::set<std::string>& seen_directives)
{
	Token		keyToken = expect(TOKEN_WORD);
	std::string	full_key = keyToken.value;

	if (peek().type == TOKEN_WORD)
	{
		Token	argToken = consume();
		full_key += " " + argToken.value;
	}

	if (seen_directives.count(full_key))
	{
		std::stringstream	ss;
		ss << "Duplicate directives found: " << full_key
			<< " at line " << keyToken.line;
		throw	ConfigException(ss.str());
	}
	seen_directives.insert(full_key);
	expect(TOKEN_COLON);

	std::string	value = "";
	while (peek().type != TOKEN_NEWLINE && peek().type != TOKEN_EOF
		&& peek().type != TOKEN_RBRACE)
	{
		Token	vToken = consume();

		if (vToken.type == TOKEN_COMMA)
			value += ",";
		else
			value += value.empty() ? vToken.value : " " + vToken.value;
	}

	if (value.empty())
		throw	ConfigException("Empty value for directive: " + full_key);
	directives_map[full_key] = value;
	if (peek().type == TOKEN_NEWLINE)
		consume();
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

	if (directives.count("redirect"))
		throw	ConfigException("'redirect' directive is not allowed in a server block.");
	if (directives.count("listen"))
	{
		std::string	value = directives["listen"];
		size_t		colon_pos = value.find(':');

		if (colon_pos != std::string::npos)
		{
			server.host = trim(value.substr(0, colon_pos));
			server.port = parsePort(trim(value.substr(colon_pos + 1)));
		} else {
			server.host = "127.0.0.1";
			server.port = parsePort(trim(value));
		}
	}
	if (directives.count("max_body_size"))
		server.max_body_size = parseSize(directives["max_body_size"]);
	if (directives.count("root"))
		server.root = directives["root"];
	if (directives.count("upload"))
		server.upload = parseOnOff(directives["upload"]);
	if (directives.count("upload_dir"))
		server.upload_dir = directives["upload_dir"];
	if (directives.count("methods"))
		server.methods = parseMethods(directives["methods"]);
	if (directives.count("index"))
		server.index = splitList(directives["index"]);

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
	for (size_t i = 0; i < server.routes.size(); ++i)
	{
		if (server.routes[i].root.empty())
			server.routes[i].root = server.root;
		if (server.routes[i].methods.empty())
			server.routes[i].methods = server.methods;
		if (server.routes[i].max_body_size == 0)
			server.routes[i].max_body_size = server.max_body_size;
		if (server.routes[i].index.empty())
			server.routes[i].index = server.index;
		std::map<std::string, std::string>::iterator	ite;
		for (ite = server.error_pages.begin(); ite != server.error_pages.end(); ++ite)
		{
			if (server.routes[i].error_pages.find(ite->first) == server.routes[i].error_pages.end())
				server.routes[i].error_pages[ite->first] = ite->second;
		}
		validateRouteConfig(server.routes[i]);
		validateCGIExecutables(server.routes[i]);
	}

	validateServerConfig(server);
	_servers.push_back(server);
}


