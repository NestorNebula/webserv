/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticResource.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 11:24:31 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/25 12:08:51 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "StaticResource.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

void StaticResource::generate() {
	if (_state != DEFAULT)
		throw std::logic_error("generate called multiple times");

	_state = (_file.is_open() && readContent()) ? DONE : FAIL;
}

const std::string &StaticResource::getContent() const {
	if (_state != DONE)
		throw std::logic_error("Content not available");
	return _content;
}

bool StaticResource::readContent() {
	std::ostringstream oss;
	if (_file.peek() != EOF) {
		oss << _file.rdbuf();
		if (_file.fail() || oss.fail())
			return false;
		_content = oss.str();
	}
	return true;
}
