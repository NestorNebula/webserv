/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Validation.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 15:35:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/22 12:43:14 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"
#include "utils/WsLog.hpp"
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

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

std::string	trim(const std::string& str)
{
	size_t	start = str.find_first_not_of(" \t");
	size_t	end = str.find_last_not_of(" \t");

	if (start == std::string::npos)
		return ("");
	return (str.substr(start, end - start + 1));
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
	if (numPart.empty())
		throw	ConfigException("Invalid max_body_size value: " + sizeStr);
	if (numPart.length() > 10)
		throw	ConfigException("max_body_size value too large: " + sizeStr);
	for (size_t i = 0; i < numPart.length(); ++i)
	{
		if (!std::isdigit(numPart[i]))
			throw	ConfigException("Invalid max_body_size value: " + sizeStr);
	}
	size_t	num = 0;
	std::stringstream	ss(numPart);
	ss >> num;
	return (num * multiplier);
}

int	ConfigParser::parsePort(const std::string& portStr)
{
	if (portStr.empty())
		throw	ConfigException("Empty port value.");
	if (portStr.length() > 5)
		throw	ConfigException("Port value out of range: " + portStr);
	for (size_t i = 0; i < portStr.length(); ++i)
	{
		if (!std::isdigit(portStr[i]))
			throw	ConfigException("Invalid port value: " + portStr);
	}
	int port = atoi(portStr.c_str());
	if (port < 1 || port > 65535)
		throw	ConfigException("Port out of range (1 - 65535): " + portStr);
	return (port);
 }

void	ConfigParser::validateServerConfig(const ServerConfig& server)
{
	if (server.port == 0)
		throw	ConfigException("Server is missing required 'listen' directive.");
	if (server.error_pages.find("default") == server.error_pages.end())
		throw	ConfigException("Server is missing required 'error_page default' directive.");
	if (server.upload && server.upload_dir.empty())
		throw	ConfigException("Server has upload enabled but no 'upload_dir'.");

	if (!server.root.empty())
		validateDirExists(server.root, "server root");
	if (server.upload && !server.upload_dir.empty())
		validateDirExists(server.upload_dir, "server upload_dir");

	std::map<std::string, std::string>::const_iterator	it;
	for (it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
		validateFileExists(it->second, "server error_page " + it->first);
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

	validateDirExists(route.root, "route '" + route.path + "' root");
	if (route.upload && !route.upload_dir.empty())
		validateDirExists(route.upload_dir, "route '" + route.path + "' upload_dir");

	std::map<std::string, std::string>::const_iterator	it;
	for (it = route.error_pages.begin(); it != route.error_pages.end(); ++it)
		validateFileExists(it->second, "route '" + route.path + "' error_page " + it->first);
}

void	ConfigParser::validateCGIExecutables(const RouteConfig& route)
{
	std::map<std::string, std::string>::const_iterator	it;
	for (it = route.cgi.begin(); it != route.cgi.end(); ++it)
	{
		if (it->second.empty() || it->second[0] != '/')
			throw	ConfigException("CGI executable must be an absolute path: "
				+ it->second);
		WsLog::_(LVL_DBG, TGT_VALIDATE, "Checking CGI executable: ", it->second);
		if (access(it->second.c_str(), X_OK) != 0)
			throw	ConfigException("CGI executable not found or not executable: "
				+ it->second + " (for extension " + it->first + ")");
	}
}

void	ConfigParser::validateDirExists(const std::string& path, const std::string& context)
{
	WsLog::_(LVL_DBG, TGT_VALIDATE, "Checking directory exists: ", path);

	struct stat	info;
	if (stat(path.c_str(), &info) != 0)
		throw	ConfigException(context + ": directory does not exist: " + path);
	if (!S_ISDIR(info.st_mode))
		throw	ConfigException(context + ": path is not a directory: " + path);
}

void	ConfigParser::validateFileExists(const std::string& path, const std::string& context)
{
	if (access(path.c_str(), R_OK) != 0)
		throw	ConfigException(context + ": file does not exist or is not readable: " + path);
}
