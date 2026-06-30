/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:35:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/30 10:24:14 by mamarti          ###   ########.fr       */
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
	(void) sizeStr;
	return (0);
}

void	ConfigParser::validateServerConfig(const ServerConfig& server)
{
	(void) server;
}

void	ConfigParser::validateRouteConfig(const RouteConfig& route)
{
	(void) route;
}
