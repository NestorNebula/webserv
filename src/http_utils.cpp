/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_utils.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 07:27:39 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/05 14:34:56 by nhoussie         ###   ########.fr       */
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

bool isCgi(const std::string &path, RouteConfig &config) {
	std::string::size_type extIndex = path.find_last_of(".");
	if (extIndex == std::string::npos || extIndex == 0
			|| path[extIndex - 1] == '/')
		return false;
	std::string extension = path.substr(extIndex);
	bool cgi = false;
	for (std::map<std::string, std::string>::const_iterator it = config.cgi.begin(), ite = config.cgi.end(); !cgi && it != ite; it++) {
		if (extension == it->first)
			cgi = true;
	}
	return cgi;
}

bool isAccessibleFile(const std::string &path) {
	return access(path.c_str(), R_OK) == 0;
}

std::string getStatusReason(Response::StatusCode code) {
	static std::map<int, std::string> reasons;

	if (reasons.empty()) {
		reasons.insert(std::pair<int, std::string>(200, "OK"));
		reasons.insert(std::pair<int, std::string>(201, "Created"));
		reasons.insert(std::pair<int, std::string>(301, "Moved Permanently"));
		reasons.insert(std::pair<int, std::string>(400, "Bad Request"));
		reasons.insert(std::pair<int, std::string>(403, "Forbidden"));
		reasons.insert(std::pair<int, std::string>(404, "Not Found"));
		reasons.insert(std::pair<int, std::string>(405, "Method Not Allowed"));
		reasons.insert(std::pair<int, std::string>(408, "Request Timeout"));
		reasons.insert(std::pair<int, std::string>(413, "Content Too Large"));
		reasons.insert(std::pair<int, std::string>(500, "Internal Server Error"));
		reasons.insert(std::pair<int, std::string>(501, "Not Implemented"));
		reasons.insert(std::pair<int, std::string>(505, "HTTP Version Not Supported"));
	}
	if (reasons.find(code) != reasons.end())
		return reasons[code];
	return std::string();
}
