/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 08:56:52 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 11:28:27 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"
#include "Stream.hpp"

class Request {
public:
  Request() : _state(EMPTY), _lineStart(0), _remainingBody(std::string::npos) {}

  // Methods
  void append(const std::string data);
  const std::string &getRaw() const { return _raw; }
  bool isComplete() const { return _state == COMPLETE; }
  bool isInvalid() const { return _state == INVALID; }
  bool hasMethod() const { return _state > START_LINE; }
  const std::string &getMethod() const { return _method; }
  bool hasURL() const { return _state > START_LINE; }
  const std::string &getURL() const { return _url; }
  bool hasQuery() const { return _state > START_LINE && !_query.empty(); }
  const std::string &getQuery() const { return _query; }
  bool hasVersion() const { return _state > START_LINE; }
  const std::string &getVersion() const { return _version; }
  bool hasHeader(const std::string &key) const {
    return _state >= HEADERS && _headers.has(key);
  }
  bool hasHeaders() const { return _state >= HEADERS && _headers.size() > 0; }
  const Headers &getHeaders() const { return _headers; }
  bool hasBody() const { return _state == COMPLETE && !_body.empty(); }
  const std::string &getBody() { return _body; }
  void clear();

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
  Stream &_body;

  std::string::size_type _lineStart;
  std::string::size_type _remainingBody;

  void handleStartLine(std::string startLine, std::string::size_type eol);
  void handleHeaderLine(std::string headerLine, std::string::size_type eol);
  void handleBody(std::string body, std::string::size_type eol);
  void handleBodyLine(std::string bodyLine, std::string::size_type eol);
};
