/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 08:56:52 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/21 10:58:08 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"

class Request {
public:
  Request() : _state(EMPTY), _lineStart(0), _remainingBody(std::string::npos){}

  // Methods
  void append(const std::string data);
  const std::string &getRaw() const;
  bool isComplete() const;
  bool isInvalid() const;
  bool hasMethod() const;
  const std::string &getMethod() const;
  bool hasURL() const;
  const std::string &getURL() const;
  bool hasQuery() const;
  const std::string &getQuery() const;
  bool hasVersion() const;
  const std::string &getVersion() const;
  bool hasHeader(const std::string &) const;
  bool hasHeaders() const;
  const Headers &getHeaders() const;
  bool hasBody() const;
  const std::string &getBody() const;
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
  std::string _body;
  std::string::size_type _lineStart;
  std::string::size_type _remainingBody;

  // Private methods
  void handleStartLine(std::string startLine, std::string::size_type eol);
  void handleHeaderLine(std::string headerLine, std::string::size_type eol);
  void handleBody(std::string body, std::string::size_type eol);
  void handleBodyLine(std::string bodyLine, std::string::size_type eol);
};
