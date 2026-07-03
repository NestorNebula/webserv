/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 10:14:17 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 10:36:52 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <fstream>
#include <sstream>

RouteConfig::RouteConfig() : autoindex(false), upload(false), max_body_size (0) {}

ServerConfig::ServerConfig() : port(0), max_body_size(1048576), upload(0)
{
	host = "127.0.0.1";
}

ConfigParser::ConfigParser() : _pos(0) {}
ConfigParser::~ConfigParser() {}

ConfigParser::ConfigException::ConfigException(const std::string& msg)
	: _msg("Config Error: " + msg) {}
ConfigParser::ConfigException::~ConfigException() throw() {}
const char*	ConfigParser::ConfigException::what() const throw() { return _msg.c_str(); }

void	ConfigParser::parseFile(const std::string& filename)
{
	// Read the file
	std::ifstream file(filename.c_str());
	if (!file.is_open())
		throw ConfigException("Cannot open configuration file.");
	std::stringstream buffer;
	buffer << file.rdbuf();

	// Convert text into Tokens
	tokenize(buffer.str());

	// Read Server blocks
	skipNewlines();
	while (peek().type != TOKEN_EOF)
	{
		Token	t = consume();
		if (t.type == TOKEN_WORD && t.value == "server")
		{
			parseServer();
		} else {
			std::stringstream	ss;
			ss << "Expected 'server' block at line " << t.line << ", got "
				<< t.value;
			throw ConfigException(ss.str());
		}
		skipNewlines();
	}
}

