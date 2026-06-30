/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:35:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/30 11:25:39 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include <sstream>

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

std::string	methodToString(const HttpMethod& method)
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
	return static_cast<size_t>(std::atoi(numPart.c_str())) * multiplier;
}

void	ConfigParser::validateServerConfig(const ServerConfig& server)
{
	(void) server;
}

void	ConfigParser::validateRouteConfig(const RouteConfig& route)
{
	(void) route;
}
