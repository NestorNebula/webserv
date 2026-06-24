/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/24 12:42:35 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/24 16:14:06 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>
#include <stdexcept>

void Response::setVersion(std::string version) {
	throwIfGenerated();
	_version = version;
}

void Response::setCode(StatusCode code) {
	throwIfGenerated();
	_code = code;
}

void Response::setReason(std::string reason) {
	throwIfGenerated();
	_reason = reason;
}

void Response::addHeader(Header header) {
	throwIfGenerated();
	_headers.insert(header);
}

void Response::addHeaders(Headers::const_iterator begin, Headers::const_iterator end) {
	throwIfGenerated();
	while (begin != end)
		_headers.insert(*begin++);
}

bool Response::isReady() const {
	return !_version.empty() && _code && !_reason.empty() && !_generated;
}

void Response::generate() {
	std::ostringstream oss;

	throwIfGenerated();
	oss << _version << " " << _code << " " << _reason << "\r\n";
	oss << _headers.str() << "\r\n";
	oss << _resource.getContent();
	_raw = oss.str();
	_generated = true;
}

void Response::throwIfGenerated() const {
	if (_generated)
		throw(std::logic_error("Response already generated"));
}
