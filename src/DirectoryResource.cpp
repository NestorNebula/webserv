/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 11:59:05 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/26 12:05:04 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryResource.hpp"
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
	dirent *dirFile;
	while ((dirFile = readdir(_dir)) != NULL)
		(_content += dirFile->d_name) += "\n";
}
