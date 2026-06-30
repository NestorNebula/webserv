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
	_version = version;
}

void Response::setCode(StatusCode code) {
	_code = code;
}

void Response::setReason(std::string reason) {
	_reason = reason;
}

void Response::addHeader(Header header) {
	_headers.insert(header);
}

void Response::addHeaders(Headers::const_iterator begin, Headers::const_iterator end) {
	while (begin != end)
		_headers.insert(*begin++);
}

const Resource &Response::getResource() const {
	if (_resource == NULL)
		throw std::logic_error("No resource available");
	return *_resource;
}

void Response::setResource(Resource *resource) {
	_resource = resource;
}

std::string Response::getHead() const {
	std::ostringstream oss;

	throwIfNotReady();
	oss << _version << " " << _code << " " << _reason << "\r\n";
	oss << _headers.str() << "\r\n";
	return oss.str();
}

Stream::streamsize Response::readBody(char *buf, Stream::streamsize bufsize) {
	throwIfNotReady();
	if (!hasBody())
		throw std::logic_error("Response does not have body");

	_resource->stream().read(buf, bufsize - 1);
	if (!_resource->stream() && !_resource->stream().eof())
		return -1;
	Stream::streamsize readCount = _resource->stream().gcount();
	buf[readCount] = '\0';
	return readCount;
}

bool Response::isReady() const {
	return !_version.empty() && _code && !_reason.empty() && (!_resource || hasBody());
}

bool Response::hasBody() const {
	return _resource && _resource->done();
}

void Response::throwIfNotReady() const {
	if (!isReady())
		throw std::logic_error("Response not in readable state");
}
