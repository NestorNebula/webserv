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
#include <stdexcept>

void StaticResource::generate() {
	if (_state != DEFAULT)
		throw std::logic_error("generate called multiple times");

	std::fstream *fs = new std::fstream(_filepath.c_str());
	_stream = new Stream(fs);
	_state = fs->is_open() ? DONE : FAIL;
}

Stream &StaticResource::stream() {
	if (!_stream || _state != DONE)
		throw std::logic_error("stream not accessible");
	return *_stream;
}
