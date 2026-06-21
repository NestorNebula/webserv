/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 11:52:03 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/20 15:48:22 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

// void Request::append(const std::string data);

const std::string &Request::getRaw() const {
	return _raw;
}

bool Request::isComplete() const {
	return _state == COMPLETE;
}

bool Request::isInvalid() const {
	return _state == INVALID;
}

bool Request::hasMethod() const {
	return _state > START_LINE;
}

const std::string &Request::getMethod() const {
	return _method;
}

bool Request::hasURL() const {
	return _state > START_LINE;
}

const std::string &Request::getURL() const {
	return _url;
}

bool Request::hasQuery() const {
	return _state > START_LINE && !_query.empty();
}

const std::string &Request::getQuery() const {
	return _query;
}

bool Request::hasVersion() const {
	return _state > START_LINE;
}

const std::string &Request::getVersion() const {
	return _version;
}

bool Request::hasHeader(const std::string &key) const {
	return _state >= HEADERS && _headers.has(key);
}
bool Request::hasHeaders() const {
	return _state >= HEADERS && _headers.size() > 0;
}

const Headers &Request::getHeaders() const {
	return _headers;
}

bool Request::hasBody() const {
	return _state == COMPLETE && !_body.empty();
}

const std::string &Request::getBody() const {
	return _body;
}
