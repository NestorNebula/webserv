/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 10:14:17 by mamarti           #+#    #+#             */
/*   Updated: 2026/08/31 11:22:44 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parsing/ConfigParser.hpp"
#include "utils/WsLog.hpp"
#include <fstream>
#include <sstream>

RouteConfig::RouteConfig() : autoindex(false), upload(false), max_body_size (0), pycgi_dir("")  {}

ServerConfig::ServerConfig() : port(0), max_body_size(1048576), upload(0), pycgi_dir("")
{
	host = "127.0.0.1";
}

ConfigParser::ConfigParser(const std::string &conf_file_root) : _pos(0), _conf_file_root(conf_file_root) {}
ConfigParser::~ConfigParser() {}

ConfigParser::ConfigException::ConfigException(const std::string& msg)
	: _msg("Config Error: " + msg) {}
ConfigParser::ConfigException::~ConfigException() throw() {}
const char*	ConfigParser::ConfigException::what() const throw() { return _msg.c_str(); }

void	ConfigParser::parseFile(const std::string& filename)
{
	WsLog::_(LVL_INFO, TGT_CONFIG, "Parsing configuration file: ", filename);

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
	if (_servers.empty())
		throw	ConfigException("No server block found in configuration.");
	WsLog::_(LVL_INFO, TGT_CONFIG, "Configuration parsed successfully, servers: ",
		static_cast<int>(_servers.size()));
}

bool	parseOnOff(const std::string& value)
{
	if (value == "on")
		return (true);
	else if (value == "off")
		return (false);
	throw	ConfigParser::ConfigException("Value must be 'on' or 'off', got: " + value);
}

