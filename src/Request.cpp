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

const std::string &Request::getMethod() const {
	return _method;
}

const std::string &Request::getURL() const {
	return _url;
}

const std::string &Request::getQuery() const {
	return _query;
}

const std::string &Request::getVersion() const {
	return _version;
}

const Headers &Request::getHeaders() const {
	return _headers;
}

const std::string &Request::getBody() const {
	return _body;
}
