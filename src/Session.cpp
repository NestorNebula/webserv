/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 08:32:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/01 12:26:21 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"
#include <cstring>

Stream::streamsize Session::write(const char *buf, Stream::streamsize count) {
	throwIfNotAction(RDSOCK);

	_request.append(std::string(buf, count));
	manageSession();
	return count;
}

void Session::setCgiResource(Resource *cgiResource) {
	throwIfNotAction(DOCGI);
	delete _resource;
	_resource = cgiResource;
	manageSession();
}

Stream::streamsize Session::read(char *buf, Stream::streamsize bufsize) {
	throwIfNotAction(WRSOCK);

	Stream::streamsize r = 0;

	std::string head = _response.getHead();
	if (static_cast<unsigned long>(_sent) < head.size()) {
		head.erase(0, _sent);
		if (head.size() > static_cast<unsigned long>(bufsize))
			head.erase(bufsize);
		std::strcpy(buf, head.c_str());
		buf += head.size();
		bufsize -= head.size();
		_sent += head.size();
		r += head.size();
	}

	if (bufsize && _response.hasBody()) {
		Stream::streamsize bodyRead = _response.readBody(buf, bufsize);
		_sent += bodyRead;
		r += bodyRead;
	}

	if (r < bufsize)
		_next = CLOSE;
	manageSession();
	return r;
}

void Session::throwIfNotAction(Action action) const {
	if (action != _next)
		throw std::logic_error("Wrong action on Session");
}

void Session::manageSession() {
	switch (_next) {
		case RDSOCK:
			if (_request.isComplete()) {
				handleRequest();
				handleResource();
			}
			if (_next != DOCGI) _next = WRSOCK;
			break;
		case DOCGI:
			if (_resource != NULL) {
				handleResource();
				_next = WRSOCK;
			}
			break;
		case WRSOCK:
			break;
		case CLOSE:
			break;
		default:
			break;
	}
}

void Session::handleRequest() {
	// TODO
	// Check Request format and validity
	// Ensure start line and headers values make sense
	// Check route and file
	// Check that method works for route/file
}

void Session::handleResource() {
	// TODO
	// Choose type of Resource depending on route/file
	// Store CGI once executed
	// Handle Resource errors
}

void Session::handleResponse() {
	// TODO
	// Add Response details and missing fields
	// Set Response headers
	// Ensure that Response is valid
}
