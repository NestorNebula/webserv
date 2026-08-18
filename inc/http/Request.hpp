/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 08:56:52 by nhoussie          #+#    #+#             */
/*   Updated: 2026/08/18 16:19:39 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"
#include "HttpMethod.hpp"
#include "Stream.hpp"
#include "WsLog.hpp"

#define MAX_BODY_SIZE (64 * 1024)
#define MAX_HEADERS_SIZE (8 * 1024)

class Request {
public:
  Request() : _state(EMPTY), _body(NULL), _bodySize(0) {
    WsLog::_(LVL_DBG, TGT_REQ, "Request constructor");
  }
  ~Request() {
    WsLog::_(LVL_DBG, TGT_REQ, "Request destructor");
    delete _body;
  }

  // Methods
  void append(const std::string &data);
  bool isComplete() const { return _state == COMPLETE; }
  bool isInvalid() const { return _state == INVALID; }
  bool hasMethod() const { return _state > START_LINE; }
  HttpMethod getMethod() const { return _method; }
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
  bool headersComplete() const { return _state > HEADERS; }
  const Headers &getHeaders() const { return _headers; }
  bool hasBody() const { return _body != NULL && _bodySize > 0; }
  Stream *getBody() {
    if (_body == NULL)
      throw std::logic_error("accessing null body Stream");
    return _body;
  }
  Stream::streamsize availableBody() const;
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
  HttpMethod _method;
  std::string _url;
  std::string _query;
  std::string _version;
  Headers _headers;
  Stream *_body;

  std::string::size_type _remainingBody;
  bool _hasLargeBody;
  std::string::size_type _bodySize;

  void handleStartLine(std::string startLine, std::string::size_type eol);
  void setMethod(const std::string &method);
  void handleHeaderLine(std::string headerLine, std::string::size_type eol);
  void setupBody();
  void handleBody(std::string body, std::string::size_type eol);
  void handleBodyLine(std::string bodyLine, std::string::size_type eol);


// #kd - get_body => ostr
public:
  std::string _ostr;
  std::string &get_body(void)
  {
    if (_ostr.size())
      return (_ostr);
    char buf[4096];
    int err = getBody()->readsome(buf, 4096);
    if (err)
      _ostr.append(buf, err);
    return (_ostr);
  }
};
