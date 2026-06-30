/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:35:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/30 11:03:35 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

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
