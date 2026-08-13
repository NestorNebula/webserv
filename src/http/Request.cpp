/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 11:52:03 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/05 14:41:57 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "TemporaryFileStream.hpp"
#include "helpers.hpp"
#include "http_utils.hpp"
#include <climits>
#include <sstream>

void Request::append(const std::string &data) {
  if (_state == COMPLETE || _state == INVALID) {
    WsLog::_(LVL_WARN, TGT_REQ, "Sending data to closed request");
    return;
  }
  WsLog::_(LVL_INFO, TGT_REQ, "Request received data: ", data);
  _raw += data;
  for (;;) {
    std::string::size_type eol(_raw.find("\r\n"));
    std::string line =
        (eol != std::string::npos) ? _raw.substr(0, eol + 2) : _raw;
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

void Request::clear() {
  _raw.clear();
  _url.clear();
  _query.clear();
  _version.clear();
  _remainingBody = std::string::npos;
  _headers.clear();
  delete _body;
  _body = NULL;
  _state = EMPTY;
  _bodySize = 0;
  _hasLargeBody = false;
}

void Request::handleStartLine(std::string startLine,
                              std::string::size_type eol) {
  if (_state == EMPTY)
    _state = START_LINE;
  if (eol == std::string::npos)
    return;
  std::istringstream iss(trim(startLine, " \t\r\n"));
  std::string methodStr;
  if (!(iss >> methodStr) || !(iss >> _url) || !(iss >> _version) ||
      !iss.eof()) {
    _state = INVALID;
    return;
  }
  setMethod(methodStr);
  std::string::size_type qIndex = _url.find("?");
  if (qIndex != std::string::npos) {
    _query = _url.substr(qIndex + 1);
    _url.erase(qIndex);
  }
  std::string decoded = decodeURI(_url);
  if (decoded.empty() || decoded[0] != '/') {
    _state = INVALID;
    return;
  }
  std::string normalized = normalizeURI(decoded);
  if (normalized.empty()) {
    _state = INVALID;
    return;
  }
  _url = normalized;
  _state = HEADERS;
  _raw.erase(0, eol + 2);
}

void Request::setMethod(const std::string &method) {
  static const std::string methods[] = {"GET", "POST", "DELETE", ""};

  int i = 0;
  while (!methods[i].empty() && method != methods[i])
    i++;
  _method = static_cast<HttpMethod>(i);
}

void Request::handleHeaderLine(std::string headerLine,
                               std::string::size_type eol) {
  if (eol == std::string::npos)
    return;
  if (headerLine == "\r\n") {
    _raw.erase(0, 2);
    setupBody();
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
  _raw.erase(0, eol + 2);
}

void Request::setupBody() {
  if (!hasHeader("Content-Length") && !hasHeader("Transfer-Encoding")) {
    _state = COMPLETE;
    return;
  }
  _state = BODY;
  if (hasHeader("Content-Length") && hasHeader("Transfer-Encoding")) {
    _state = INVALID;
    return;
  }
  if (hasHeader("Content-Length")) {
    bool err;
    _remainingBody = getLong(_headers.find("Content-Length")->second.c_str(),
                             &err, 0, INT_MAX);
    if (err) {
      _state = INVALID;
      return;
    }
  } else if (hasHeader("Transfer-Encoding")) {
    if (_headers.find("Transfer-Encoding")->second != "chunked") {
      _state = INVALID;
      return;
    }
    _remainingBody = std::string::npos;
  }
  _body = new Stream(new std::stringstream);
  _hasLargeBody = false;
  _bodySize = 0;
}

void Request::handleBody(std::string body, std::string::size_type eol) {
  if (!_hasLargeBody && _bodySize > MAX_BODY_SIZE) {
    TemporaryFileStream *bodyFile = new TemporaryFileStream();
    *bodyFile << _body->rdbuf();
    delete _body;
    _body = bodyFile;
    _hasLargeBody = true;
  }
  if (_headers.has("Content-Length")) {
    if (body.size() > _remainingBody) {
      _state = INVALID;
      return;
    }
    *_body << body;
    _bodySize += body.size();
    _remainingBody -= body.size();
    if (_remainingBody == 0) {
      _state = COMPLETE;
    } else if (eol != std::string::npos) {
      _raw.erase(0, eol + 2);
    } else
      _raw.clear();
  } else if (_headers.has("Transfer-Encoding"))
    handleBodyLine(body, eol);
  else
    _state = INVALID;
}

void Request::handleBodyLine(std::string bodyLine, std::string::size_type eol) {
  if (eol == std::string::npos)
    return;
  if (bodyLine == "\r\n") {
    _state = _remainingBody == 0 ? COMPLETE : INVALID;
    return;
  }
  if (_remainingBody == std::string::npos) {
    bool err;
    _remainingBody = getLong(bodyLine.c_str(), &err, 0, INT_MAX, 16, '\r');
    if (err) {
      _state = INVALID;
      return;
    }
  } else if (eol != _remainingBody) {
    _state = INVALID;
  } else {
    _body->write(bodyLine.c_str(), eol);
    _bodySize += _remainingBody;
    _remainingBody = std::string::npos;
  }
  _raw.erase(0, eol + 2);
}
