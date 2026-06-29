/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 11:59:05 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 14:27:41 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryResource.hpp"
#include <sstream>
#include <stdexcept>

void DirectoryResource::generate() {
	if (_state != DEFAULT)
		throw std::logic_error("generate called multiple times");
	
	if (_dir)
		buildList();
	_state = (_dir != NULL) ? DONE : FAIL;
}

const std::string &DirectoryResource::getContent() const {
	if (_state != DONE)
		throw std::logic_error("Content not available");
	return _content;
}

void DirectoryResource::buildList() {
	std::ostringstream oss;

	oss << "<!DOCTYPE html>\n"
		"<html lang=\"en\">\n"
		"<head>\n"
		"<meta charset=\"utf-8\">\n"
		"<title>Directory listing for " << _dirpath << "</title>\n"
		"</head>\n"
		"<body>\n"
		"<h1>Directory listing for " << _dirpath << "</h1>\n"
		"<hr>\n"
		"<ul>\n";

	dirent *dirFile;
	while ((dirFile = readdir(_dir)) != NULL) {
		std::string name(dirFile->d_name);
		if (name != "." && name != "..") {
			if (dirFile->d_type ==  DT_DIR)
				name += '/';
			oss << "<li>\n" 
				"<a href=\"" << name << "\">" << name << "</a>\n"
				"</li>\n";
		}
	}
	oss << "</ul>\n"
		"<hr>\n"
		"</body>\n"
	"</html>\n";
	_content = oss.str();
}
