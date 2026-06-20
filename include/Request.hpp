/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 08:56:52 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/20 15:36:25 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"

class Request {
public:
	Request(): _state(EMPTY) {}

	// Methods
	void append(const std::string data);
	const std::string &getRaw() const;
	bool isComplete() const;
	bool isInvalid() const;
	const std::string &getMethod() const;
	const std::string &getURL() const;
	const std::string &getQuery() const;
	const std::string &getVersion() const;
	const Headers &getHeaders() const;
	const std::string &getBody() const;

private:
	Request(const Request &);
	Request &operator=(const Request &);
	typedef enum eInternalState {
		EMPTY,
		START_LINE,
		HEADERS,
		BODY,
		COMPLETE,
		INVALID,
	} InternalState;
	std::string _raw;
	InternalState _state;
	std::string _method;
	std::string _url;
	std::string _query;
	std::string _version;
	Headers _headers;
	std::string _body;
};
