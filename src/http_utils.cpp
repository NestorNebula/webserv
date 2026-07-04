/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_utils.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 07:27:39 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/03 08:01:54 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_utils.hpp"
#include <sys/stat.h>
#include <unistd.h>

bool isAllowedMethod(HttpMethod method, RouteConfig &config) {
	return config.methods.find(method) != config.methods.end();
}

bool isValidVersion(const std::string &version) {
	std::string::size_type expectedSize = std::string("HTTP/x.x").size();
	if (version.size() != expectedSize || version.find("HTTP/") == std::string::npos)
		return false;
	std::string::size_type majorIndex = expectedSize - 3;
	if (majorIndex == std::string::npos)
		return false;
	if (version[majorIndex] != '1'
			|| version[majorIndex + 1] != '.'
			|| (version[majorIndex + 2] <'0' || version[majorIndex + 2] > '1'))
		return false;
	return true;
}

RouteConfig *findBestRoute(const std::string &url, ServerConfig &config) {
	RouteConfig *bestRoute = NULL;

	for (std::vector<RouteConfig>::iterator it = config.routes.begin(), ite = config.routes.end(); it != ite; it++) {
		std::string::size_type equalSize = 0;
		while (equalSize < url.size() && equalSize < it->path.size() && url[equalSize] == it->path[equalSize])
			++equalSize;
		if ((equalSize == it->path.size() && ((url.size() == equalSize) || url[equalSize] == '/'))
				&& (!bestRoute || equalSize > bestRoute->path.size())) {
			bestRoute = &(*it);
		}
	}
	return bestRoute;
}

std::string resolvePath(const std::string &url, RouteConfig &config) {
	std::string path(config.root);
	if (url.size() > config.path.size())
		path += url.substr(config.path.size());
	return path;
}

bool isExistingFile(const std::string &path) {
	return access(path.c_str(), F_OK) == 0;
}

bool isDirectory(const std::string &path) {
	struct stat statbuf;

	if (stat(path.c_str(), &statbuf) != 0)
		return false; 
	return S_ISDIR(statbuf.st_mode);
}

bool isCgi(const std::string &path, RouteConfig &config); // TODO: Check CGI config in config file

bool isAccessibleFile(const std::string &path) {
	return access(path.c_str(), R_OK) == 0;
}
