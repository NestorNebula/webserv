/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 11:52:03 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/21 15:56:50 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "helpers.hpp"
#include <climits>
#include <cstdlib>
#include <sstream>

void Request::append(const std::string data) {
  if (_state == COMPLETE || _state == INVALID)
    return;
  _raw += data;
  for (;;) {
    std::string::size_type eol(_raw.find("\r\n", _lineStart));
    std::string line = (eol != std::string::npos)
                           ? _raw.substr(_lineStart, eol - _lineStart + 2)
                           : _raw.substr(_lineStart);
    switch (_state) {
    case EMPTY:
    case START_LINE:
      handleStartLine(line, eol);
      break;
    case HEADERS:
      handleHeaderLine(line, eol);
      break;
    case BODY:
      handleBody(line, eol);
      break;
    default:
      return;
    }
    if (eol == std::string::npos)
      break;
  }
}

const std::string &Request::getRaw() const { return _raw; }

bool Request::isComplete() const { return _state == COMPLETE; }

bool Request::isInvalid() const { return _state == INVALID; }

bool Request::hasMethod() const { return _state > START_LINE; }

const std::string &Request::getMethod() const { return _method; }

bool Request::hasURL() const { return _state > START_LINE; }

const std::string &Request::getURL() const { return _url; }

bool Request::hasQuery() const {
  return _state > START_LINE && !_query.empty();
}

const std::string &Request::getQuery() const { return _query; }

bool Request::hasVersion() const { return _state > START_LINE; }

const std::string &Request::getVersion() const { return _version; }

bool Request::hasHeader(const std::string &key) const {
  return _state >= HEADERS && _headers.has(key);
}
bool Request::hasHeaders() const {
  return _state >= HEADERS && _headers.size() > 0;
}

const Headers &Request::getHeaders() const { return _headers; }

bool Request::hasBody() const { return _state == COMPLETE && !_body.empty(); }

const std::string &Request::getBody() const { return _body; }

void Request::handleStartLine(std::string startLine,
                              std::string::size_type eol) {
  if (_state == EMPTY)
    _state = START_LINE;
  if (eol == std::string::npos)
    return;
  std::istringstream iss(trim(startLine, " \t\r\n"));
  if (!(iss >> _method) || !(iss >> _url) || !(iss >> _version) || !iss.eof()) {
    _state = INVALID;
    return;
  }
  std::string::size_type qIndex = _url.find("?");
  if (qIndex != std::string::npos) {
    _query = _url.substr(qIndex + 1);
    _url.erase(qIndex);
  }
  _state = HEADERS;
  _lineStart = eol + 2;
}
void Request::handleHeaderLine(std::string headerLine,
                               std::string::size_type eol) {
  if (eol == std::string::npos)
    return;
  if (headerLine == "\r\n") {
    _lineStart = eol + 2;
    _state = (hasHeader("Content-Length") || hasHeader("Transfer-Encoding"))
                 ? BODY
                 : COMPLETE;
    if (_state == BODY && hasHeader("Content-Length"))
      _remainingBody =
          std::atoi(_headers.find("Content-Length")->second.c_str());
    return;
  }
  std::string::size_type sep = headerLine.find(":");
  if (sep == std::string::npos) {
    _state = INVALID;
    return;
  }
  Header header(trim(headerLine.substr(0, sep), " \t\r\n"),
                trim(headerLine.substr(sep + 1), " \t\r\n"));
  if (header.first.empty() || header.second.empty() ||
      hasHeader(header.first)) {
    _state = INVALID;
    return;
  }
  _headers.insert(header);
  _lineStart = eol + 2;
}

void Request::handleBody(std::string body, std::string::size_type eol) {
  if (_headers.has("Content-Length")) {
    if (body.size() > _remainingBody) {
      _state = INVALID;
      return;
    }
    _body += body;
    _remainingBody -= body.size();
    if (_remainingBody == 0)
      _state = COMPLETE;
    else
      _lineStart += body.size();
  } else if (_headers.has("Transfer-Encoding"))
    handleBodyLine(body, eol);
  else
    _state = INVALID;
}

void Request::handleBodyLine(std::string bodyLine, std::string::size_type eol) {
  static std::string::size_type chunkSize = std::string::npos;
  if (eol == std::string::npos)
    return;
  if (bodyLine == "\r\n") {
    _state = COMPLETE;
    return;
  }
  if (chunkSize == std::string::npos) {
    char *endptr = NULL;
    chunkSize = std::strtol(bodyLine.c_str(), &endptr, 16);
    if (*endptr != '\r' || chunkSize > INT_MAX) {
      _state = INVALID;
      return;
    }
  } else if (eol - _lineStart != chunkSize) {
    _state = INVALID;
  } else {
    _body.append(bodyLine, 0, eol - _lineStart);
    chunkSize = std::string::npos;
  }
}
