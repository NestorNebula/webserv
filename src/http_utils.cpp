/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_utils.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 07:27:39 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 11:54:20 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.hpp"
#include "http_utils.hpp"
#include <sys/stat.h>
#include <unistd.h>

bool isAllowedMethod(HttpMethod method, RouteConfig &config) {
	return config.methods.find(method) != config.methods.end();
}

bool isValidVersion(const std::string &version) {
	std::string::size_type expectedSize = std::string("HTTP/x.x").size();
	if ((version.size() != expectedSize && version.size() != expectedSize - 2) || version.find("HTTP/") != 0)
		return false;
	if (version.size() == expectedSize - 2)
		return version[version.size() - 1] >= '0' && version[version.size() - 1] <= '9';
	std::string::size_type majorIndex = expectedSize - 3;
	if (majorIndex == std::string::npos)
		return false;
	unsigned char ma = version[majorIndex], mi = version[majorIndex + 2];
	if ((ma < '0' || ma > '9')
			|| version[majorIndex + 1] != '.'
			|| (mi < '0' || mi > '9'))
		return false;
	return true;
}

RouteConfig *findBestRoute(const std::string &url, ServerConfig &config) {
	RouteConfig *bestRoute = NULL;

	for (std::vector<RouteConfig>::iterator it = config.routes.begin(), ite = config.routes.end(); it != ite; it++) {
		std::string::size_type equalSize = 0;
		while (equalSize < url.size() && equalSize < it->path.size() && url[equalSize] == it->path[equalSize])
			++equalSize;
		if ((equalSize == it->path.size() && ((url.size() == equalSize) || (url[equalSize] == '/') || (it->path[equalSize - 1] == '/' && url[equalSize - 1] == '/')))
				&& (!bestRoute || equalSize > bestRoute->path.size())) {
			bestRoute = &(*it);
		}
	}
	return bestRoute;
}

std::string resolvePath(const std::string &url, RouteConfig &config) {
	std::string path(config.root);
	if (url.size() > config.path.size())
		path = joinPaths(path, url.substr(config.path.size()));
	return path;
}

std::string joinPaths(const std::string &prefix, const std::string &suffix) {
	if (prefix.empty() || suffix.empty())
		return prefix + suffix;
	return  trim(prefix, "/", false) + "/" + trim(suffix, "/", true, false);
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
		reasons[200] = "OK";
		reasons[201] = "Created";
		reasons[204] = "No Content";
		reasons[301] = "Moved Permanently";
		reasons[400] = "Bad Request";
		reasons[403] = "Forbidden";
		reasons[404] = "Not Found";
		reasons[405] = "Method Not Allowed";
		reasons[408] = "Request Timeout";
		reasons[413] = "Content Too Large";
		reasons[431] = "Request Header Fields Too Large";
		reasons[500] = "Internal Server Error";
		reasons[501] = "Not Implemented";
		reasons[505] = "HTTP Version Not Supported";
	}
	std::map<int, std::string>::const_iterator reason = reasons.find(code);
	if (reason != reasons.end())
		return reason->second;
	return std::string();
}

std::string getMimeType(const std::string &path) {
	static std::map<std::string, std::string> types;
	std::string::size_type extIndex = path.find_last_of('.');
	std::string ext = (extIndex != std::string::npos && extIndex != 0 && extIndex != path.size() - 1)
		? path.substr(extIndex + 1)
		: "";
	for (std::string::iterator it = ext.begin(), ite = ext.end(); it != ite; it++)
		*it = std::tolower(static_cast<unsigned char>(*it));

	if (types.empty()) {
		// Application
		types["js"] = "application/javascript";
		types["json"] = "application/json";
		types["pdf"] = "application/pdf";
		types["zip"] = "application/zip";
		types["gz"] = "application/gzip";
		types["xml"] = "application/xml";

		// Audio
		types["mp3"] = "audio/mpeg";
		types["wav"] = "audio/wav";
		types["ogg"] = "audio/ogg";

		// Font
		types["woff"] = "font/woff";
		types["woff2"] = "font/woff2";
		types["ttf"] = "font/ttf";
		types["otf"] = "font/otf";

		// Images
		types["png"] = "image/png";
		types["jpg"] = "image/jpeg";
		types["jpeg"] = "image/jpeg";
		types["gif"] = "image/gif";
		types["svg"] = "image/svg+xml";
		types["ico"] = "image/x-icon";
		types["webp"] = "image/webp";

		// Text
		types["html"] = "text/html";
		types["htm"] = "text/html";
		types["css"] = "text/css";
		types["txt"] = "text/plain";
	
		// Video
		types["mp4"] = "video/mp4";
		types["webm"] = "video/webm";
	}

	std::map<std::string, std::string>::const_iterator type = types.find(ext);
	if (type != types.end())
		return type->second;
	return "application/octet-stream";
}

std::string decodeURI(const std::string &uri) {
	static const std::string unreserved("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~");
	static const std::string hexa("0123456789ABCDEF");
	std::string decoded;

	for (std::string::const_iterator it = uri.begin(), ite = uri.end(); it != ite; it++) {
		if (*it != '%') {
			decoded.push_back(*it);
			continue;
		}
		if (ite - it <= 2)
			return std::string();
		std::string::size_type h1 = hexa.find(std::toupper(static_cast<unsigned char>(*(it + 1)))), h2 = hexa.find(std::toupper(static_cast<unsigned char>(*(it + 2))));
		if (h1 == std::string::npos || h2 == std::string::npos)
			return std::string();
		unsigned char value = h1 * hexa.size() + h2;
		if (unreserved.find(std::toupper(value)) == std::string::npos)
			decoded.push_back(*it);
		else {
			decoded.push_back(value);
			it += 2;
		}
	}
	return decoded;
}

std::string encodeURI(const std::string &uri) {
	static const std::string unreserved("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~");
	static const std::string hexa("0123456789ABCDEF");
	std::string encoded;

	for (std::string::const_iterator it = uri.begin(), ite = uri.end(); it != ite; it++) {
		unsigned char c = *it;
		if (unreserved.find(std::toupper(c)) != std::string::npos || *it == '/')
			encoded.push_back(c);
		else {
			encoded.push_back('%');
			encoded.push_back(hexa[c / hexa.size()]);
			encoded.push_back(hexa[c % hexa.size()]);
		}
	}
	return encoded;
}

std::string normalizeURI(const std::string &uri) {
	std::vector<std::string> splitUri(split(uri, "/"));
	std::vector<std::string> v;
	bool endSlash = false;

	for (std::vector<std::string>::const_iterator it = splitUri.begin(), ite = splitUri.end(); it != ite; it++) {
		if (*it == ".." && !v.empty())
			v.pop_back();
		else if (*it != ".." && *it != ".") {
			v.push_back(*it);
			endSlash = it + 1 != ite || uri[uri.size() - 1] == '/';
		}
	}

	if (v.empty())
		return "/";
	std::string normalized("/");
	for (std::vector<std::string>::const_iterator it = v.begin(), ite = v.end(); it != ite; it++) {
		(normalized += *it) += "/";
	}
	if (!endSlash)
		normalized.erase(normalized.size() - 1);
	return normalized;
}
