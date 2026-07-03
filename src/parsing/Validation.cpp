/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:35:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 13:38:07 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <sstream>
#include <cstdlib>

#define GET		"GET"
#define POST	"POST"
#define DELETE	"DELETE"

HttpMethod	stringToMethod(const std::string& string)
{
	if (string == GET)
		return (METHOD_GET);
	if (string == POST)
		return (METHOD_POST);
	if (string == DELETE)
		return (METHOD_DELETE);
	return (METHOD_UNKNOWN);
}

std::string	methodToString(HttpMethod method)
{
	switch (method)
	{
		case	METHOD_GET:		return ("GET");
		case	METHOD_POST:	return ("POST");
		case	METHOD_DELETE:	return ("DELETE");
		default:				return ("UNKNOWN");
	}
}

std::set<HttpMethod>	parseMethods(const std::string& value)
{
	std::set<HttpMethod>	result;
	std::stringstream		ss(value);
	std::string				token;

	while (std::getline(ss, token, ','))
	{
		size_t	start = token.find_first_not_of(" \t");
		size_t	end = token.find_last_not_of(" \t");
		if (start == std::string::npos)
			continue;
		std::string	clean = token.substr(start, end - start + 1);

		HttpMethod	method = stringToMethod(clean);
		if (method == METHOD_UNKNOWN)
			throw	ConfigParser::ConfigException("Invalid HTTP method: " + clean);
		result.insert(method);
	}
	return (result);
}

std::vector<std::string>	splitList(const std::string& value)
{
	std::vector<std::string>	result;
	std::stringstream			ss(value);
	std::string					token;

	while (std::getline(ss, token, ','))
	{
		size_t	start = token.find_first_not_of(" \t");
		size_t	end = token.find_last_not_of(" \t");
		if (start == std::string::npos)
			continue;
		result.push_back(token.substr(start, end - start + 1));
	}
	return (result);
}

size_t	ConfigParser::parseSize(const std::string& sizeStr)
{
	if (sizeStr.empty())
		throw	ConfigException("Empty max_body_size value.");

	size_t		multiplier = 1;
	std::string	numPart = sizeStr;
	char		lastChar = sizeStr[sizeStr.length() - 1];

	if (lastChar == 'm' || lastChar == 'M')
	{
		multiplier = 1024 * 1024;
		numPart = sizeStr.substr(0, sizeStr.length() - 1);
	} else if (lastChar == 'k' || lastChar == 'K') {
		multiplier = 1024;
		numPart = sizeStr.substr(0, sizeStr.length() - 1);
	}

	for (size_t i = 0; i < numPart.length(); ++i)
	{
		if (!std::isdigit(numPart[i]))
			throw	ConfigException("Invalid max_body_size value:" + sizeStr);
	}
	return static_cast<size_t>(atoi(numPart.c_str())) * multiplier;
}

void	ConfigParser::validateServerConfig(const ServerConfig& server)
{
	if (server.port == 0)
		throw	ConfigException("Server is missing required 'listen' directive.");
	if (server.error_pages.find("default") == server.error_pages.end())
		throw	ConfigException("Server is missing required 'error_page default' directive.");
	if (server.upload && server.upload_dir.empty())
		throw	ConfigException("Server has upload enabled but no 'upload_dir'.");
}

void	ConfigParser::validateRouteConfig(const RouteConfig& route)
{
	if (route.path.empty())
		throw	ConfigException("Route has an empty path");
	if (route.root.empty())
		throw	ConfigException("Route '" + route.path + "' has no 'root' (and none to inherit).");
	if (route.methods.empty())
		throw	ConfigException("Route '" + route.path + "' has no 'methods' (and none to inherit).");
	if (route.upload && route.upload_dir.empty())
		throw	ConfigException("Route '" + route.path + "' has upload route enabled but no 'upload_dir'.");
}
