/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 11:52:03 by nhoussie          #+#    #+#             */
/*   Updated: 2026/09/09 11:27:19 by kdonlon          ###   ########.fr       */
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
    WSLOG(LVL_WARN, TGT_REQ, "Sending data to closed request");
    return;
  }
  //WSLOG(LVL_INFO, TGT_REQ, "Request received data: ", data);
  _raw += data;
  for (;;) {
    if (_state == BODY) {
      std::string::size_type oldSize = _raw.size();
      if (hasHeader("Transfer-Encoding")) {
        handleChunkedBody();
      } else {
        handleBody();
      }
      if (_state == COMPLETE || _state == INVALID || oldSize == _raw.size())
        return;
      continue;
    }

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

void Request::handleBody() {
  if (!_hasLargeBody && _bodySize > MAX_BODY_SIZE) {
    TemporaryFileStream *bodyFile = new TemporaryFileStream(*_body);
    delete _body;
    _body = bodyFile;
    _hasLargeBody = true;
  }

  if (_raw.size() >= _remainingBody) {
    _body->write(_raw.c_str(), _remainingBody);
    _bodySize += _remainingBody;
    _raw.erase(0, _remainingBody);
    _remainingBody = 0;
  } else {
    _body->write(_raw.c_str(), _raw.size());
    _bodySize += _raw.size();
    _remainingBody -= _raw.size();
    _raw.clear();
  }
  if (_remainingBody == 0) {
    _state = COMPLETE;
  }
}

void Request::handleChunkedBody() {
  if (!_hasLargeBody && _bodySize > MAX_BODY_SIZE) {
    TemporaryFileStream *bodyFile = new TemporaryFileStream(*_body);
    delete _body;
    _body = bodyFile;
    _hasLargeBody = true;
  }

  if (_remainingBody == std::string::npos) {
    std::string::size_type eol = _raw.find("\r\n");
    if (eol == std::string::npos)
      return;

    bool err = false;
    _chunkSize = getLong(_raw.substr(0, eol).c_str(), &err, 0, INT_MAX, 16);
    if (err) {
      _state = INVALID;
      return;
    }
    _remainingBody = _chunkSize;

    _raw.erase(0, eol + 2);
  }

  if (_remainingBody > 0) {
    std::string::size_type dataSize = (_raw.size() > _remainingBody)
      ? _remainingBody
      : _raw.size();

    if (dataSize > 0) {
      _body->write(_raw.c_str(), dataSize);
      _bodySize += dataSize;
      _remainingBody -= dataSize;
      _raw.erase(0, dataSize);
    }
    if (_remainingBody > 0)
      return;
  }

  if (_raw.size() < 2)
    return;
  if (_raw.find("\r\n") != 0) {
    _state = INVALID;
    return;
  }
  _raw.erase(0, 2);

  if (_chunkSize == 0) {
    _state = COMPLETE;
    _headers.remove("Transfer-Encoding");
    _headers.insert("Content-Length", toString(_bodySize));
  }
  _remainingBody = std::string::npos;
}

bool Request::keepalive() const {
  if (!hasVersion() || !isValidVersion(_version))
    return false;
  if (hasHeader("Connection") && _headers.find("Connection")->second == "keep-alive")
    return true;
  return _version == "HTTP/1.1";
}

Stream::streamsize Request::availableBody() const {
  if (!hasBody())
    return (0);
  return _body->size() - _body->tellg();
}
