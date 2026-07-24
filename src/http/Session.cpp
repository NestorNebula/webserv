/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 08:32:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 15:24:58 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"
#include "BuiltinResource.hpp"
#include "DirectoryResource.hpp"
#include "HttpMethod.hpp"
#include "Request.hpp"
#include "StaticResource.hpp"
#include "helpers.hpp"
#include "http_utils.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

Stream::streamsize Session::write(const char *buf, Stream::streamsize count) {
  throwIfNotAction(RDSOCK);

  _request.append(std::string(buf, count));
  std::ostringstream oss;
  oss << "Session received " << count << " bytes of data";
  WsLog::_(LVL_INFO, TGT_SESS_WR, oss.str());
  manageSession();
  return count;
}

const Request &Session::getRequest() const {
  throwIfNotAction(DOCGI);
  return _request;
}

Session::CgiInfo Session::getCgiInfo() const {
  throwIfNotAction(DOCGI);
  CgiInfo info;

  info.scriptPath = _resourcePath;
  std::string ext = _resourcePath.substr(_resourcePath.find_last_of("."));
  std::map<std::string, std::string>::const_iterator it = _route->cgi.find(ext);
  info.executablePath = it->second;
  return info;
}

void Session::setCgiResource(Resource *cgiResource) {
  throwIfNotAction(DOCGI);
  delete _resource;
  _resource = cgiResource;
  WsLog::_(LVL_INFO, TGT_SESS, "Session received CGI resource");
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
    std::strncpy(buf, head.c_str(), head.size());
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
  std::ostringstream oss;
  oss << "Session sending " << r << "bytes of data";
  WsLog::_(LVL_INFO, TGT_SESS_RD, oss.str());
  manageSession();
  return r;
}

void Session::reset() {
  throwIfNotAction(KPALIVE);

  _sent = 0;

  _request.clear();
  _response.clear();
  delete _resource;
  _resource = NULL;

  _next = RDSOCK;
}

void Session::throwIfNotAction(Action action) const {
  if (action != _next)
    throw std::logic_error("Wrong action on Session");
}

void Session::manageSession() {
  switch (_next) {
  case RDSOCK:
    handleRequest();
    if (_request.isComplete() || _request.isInvalid() || _response.getCode()) {
      handleResource();
      if (_next != DOCGI) {
        handleResponse();
        _next = WRSOCK;
      }
    }
    break;
  case DOCGI:
    if (_resource != NULL) {
      handleResponse();
      _next = WRSOCK;
    }
    break;
  case WRSOCK:
    break;
  case CLOSE:
  case KPALIVE:
    if (_keepalive)
      _next = KPALIVE;
    else
      _next = CLOSE;
    break;
  default:
    break;
  }
}

void Session::handleRequest() {
  preValidateRequest();
  if (!_request.isComplete() && !_request.isInvalid())
    return;
  validateRequest();
  resolveResource();
  validateOperation();
}

void Session::preValidateRequest() {
  WsLog::_(LVL_INFO, TGT_SESS, "Pre-validating Session Request");
  if (_request.hasVersion() && !isValidVersion(_request.getVersion()))
    return setResponseStatus(400);
  if (_request.hasVersion() && _request.getVersion() != "HTTP/1.0" &&
      _request.getVersion() != "HTTP/1.1")
    return setResponseStatus(505);
  if (_request.hasMethod() && _request.getMethod() == METHOD_UNKNOWN)
    return setResponseStatus(501);
  if (_request.hasBody() &&
      static_cast<unsigned int>(_request.getBody()->size()) >
          _server.max_body_size)
    return setResponseStatus(413);
  WsLog::_(LVL_INFO, TGT_SESS, "Session Request pre-validation successful");
}

void Session::validateRequest() {
  WsLog::_(LVL_INFO, TGT_SESS, "Validating Session Request");
  if (_request.hasHeaders() &&
      _request.getHeaders().str().size() > MAX_HEADERS_SIZE)
    return setResponseStatus(431);
  if (_request.isInvalid() || (_request.getVersion() == "HTTP/1.1" &&
                               !_request.getHeaders().has("Host")))
    return setResponseStatus(400);
  WsLog::_(LVL_INFO, TGT_SESS, "Session Request validation successful");
}

void Session::resolveResource() {
  if (_response.getCode())
    return;
  WsLog::_(LVL_INFO, TGT_SESS, "Resolving Request Resource");
  _route = findBestRoute(_request.getURL(), _server);
  if (!_route)
    return setResponseStatus(404);
  WsLog::_(LVL_INFO, TGT_SESS, "Request route found: ", _route->path);
  if (!_route->redirect.empty())
    return setResponseStatus(301);
  if (!isAllowedMethod(_request.getMethod(), *_route))
    return setResponseStatus(405);

  _resourcePath = resolvePath(_request.getURL(), *_route);
  WsLog::_(LVL_INFO, TGT_SESS, "Request Resource resolved: ", _resourcePath);
  if (isCgi(_resourcePath, *_route)) {
    _next = DOCGI;
    return;
  }
}

void Session::validateOperation() {
  if (_response.getCode())
    return;
  if (_next == DOCGI) {
    if (!isAccessibleFile(_resourcePath, X_OK))
      return setResponseStatus(403);
    return;
  }
  WsLog::_(LVL_INFO, TGT_SESS,
           "Checking operation is possible on Session Resource");
  if (_request.getMethod() == METHOD_POST && _route->upload) {
    if (isExistingFile(_resourcePath))
      return setResponseStatus(403);
    return;
  }
  if (!isExistingFile(_resourcePath))
    return setResponseStatus(404);
  if (!isAccessibleFile(_resourcePath, R_OK))
    return setResponseStatus(403);
  WsLog::_(LVL_INFO, TGT_SESS, "Operation possible on Session Resource");
}

void Session::handleResource() {
  if (_next == DOCGI)
    return;
  WsLog::_(LVL_INFO, TGT_SESS, "Preparing Session Resource generation");
  if (_response.getCode() >= 400)
    prepareErrorResource();
  else if (!_response.getCode()) {
    switch (_request.getMethod()) {
    case METHOD_GET:
      if (isDirectory(_resourcePath)) {
        if (_request.getURL().find_last_of('/') == _request.getURL().size() - 1)
          prepareDirectoryResource();
        else
          setResponseStatus(301);
      } else
        _resource = new StaticResource(_resourcePath);
      break;
    case METHOD_POST:
      handleUpload();
      break;
    case METHOD_DELETE:
      handleDelete();
      break;
    default:
      break;
    }
  }
  if (!_resource && _response.getCode() != 204)
    _resource = new BuiltinResource(_response.getCode());
  // Generate Resource
  if (_resource) {
    WsLog::_(LVL_INFO, TGT_SESS, "Generating Session Resource");
    _resource->generate();
    // Handle Resource errors
    if (_resource->failed()) {
      setResponseStatus(500);
      delete _resource;
      _resource = NULL;
      WsLog::_(LVL_ERR, TGT_SESS, "Error when generating Session Resource");
    } else
      WsLog::_(LVL_INFO, TGT_SESS, "Session Resource generated successfully");
  }
}

void Session::prepareErrorResource() {
  std::map<std::string, std::string> errPages =
      !_route ? _server.error_pages : _route->error_pages;
  std::string codeStr = toString(_response.getCode());
  std::string errPage =
      joinPaths(_server.root, errPages.find(codeStr) != errPages.end()
                                  ? errPages[codeStr]
                                  : errPages["default"]);
  if (!isAccessibleFile(errPage, R_OK))
    errPage = joinPaths(_server.root, errPages["default"]);
  delete _resource;
  WsLog::_(LVL_INFO, TGT_SESS, "Generating Error page Resource using ",
           errPage);
  _resource = new StaticResource(errPage);
  _resourcePath = errPage;
}

void Session::prepareDirectoryResource() {
  WsLog::_(LVL_INFO, TGT_SESS,
           "Processing requested Directory: ", _resourcePath);
  bool indexFound = false;
  for (std::vector<std::string>::const_iterator it = _route->index.begin(),
                                                ite = _route->index.end();
       it != ite && !indexFound; it++) {
    std::string indexPath = joinPaths(_resourcePath, *it);
    if (isAccessibleFile(indexPath, R_OK)) {
      _resourcePath = indexPath;
      indexFound = true;
    }
  }
  if (indexFound) {
    WsLog::_(LVL_INFO, TGT_SESS,
             "Preparing StaticResource with Directory index: ", _resourcePath);
    _resource = new StaticResource(_resourcePath);
  } else if (_route->autoindex) {
    WsLog::_(LVL_INFO, TGT_SESS, "Preparing DirectoryResource");
    _resource = new DirectoryResource(_resourcePath);
  } else {
    setResponseStatus(403);
    prepareErrorResource();
  }
}

void Session::handleUpload() {
  WsLog::_(LVL_INFO, TGT_SESS, "Processing upload Request");
  std::string uploadDir = joinPaths(_route->root, _route->upload_dir);
  if (!isDirectory(uploadDir))
    return setResponseStatus(400);
  std::string uploadFile =
      joinPaths(uploadDir, _request.getURL().substr(_route->path.size()));
  if (isExistingFile(uploadFile))
    return setResponseStatus(403);
  std::ofstream ofs(uploadFile.c_str());
  if (!ofs.is_open())
    return setResponseStatus(500);
  Stream *bodyStream = _request.hasBody() ? _request.getBody() : NULL;
  WsLog::_(LVL_INFO, TGT_SESS, "Starting file upload on: ", uploadFile);
  if (bodyStream) {
    char buf[BUFSIZE];
    while (bodyStream->read(buf, BUFSIZE))
      ofs.write(buf, bodyStream->gcount());
    if (bodyStream->eof() && bodyStream->gcount())
      ofs.write(buf, bodyStream->gcount());
    if (!bodyStream->eof() || !ofs) {
      WsLog::_(LVL_ERR, TGT_SESS, "Error during file upload, aborting");
      ofs.close();
      std::remove(uploadFile.c_str());
      return setResponseStatus(500);
    }
  }
  ofs.close();
  WsLog::_(LVL_INFO, TGT_SESS, "File uploaded successfully");
  setResponseStatus(201);
}

void Session::handleDelete() {
  WsLog::_(LVL_INFO, TGT_SESS,
           "Processing DELETE Request for: ", _resourcePath);
  if (isDirectory(_resourcePath))
    return setResponseStatus(403);
  errno = 0;
  if (std::remove(_resourcePath.c_str()) != 0) {
    if (errno == EACCES)
      return setResponseStatus(403);
    else
      return setResponseStatus(500);
  }
  WsLog::_(LVL_INFO, TGT_SESS, "File deleted successfully");
  setResponseStatus(204);
}

void Session::handleResponse() {
  WsLog::_(LVL_INFO, TGT_SESS, "Preparing Session Response");
  // Add Response details and missing fields
  if (!_request.hasVersion() || !isValidVersion(_request.getVersion()))
    _response.setVersion("HTTP/1.1");
  else
    _response.setVersion(_request.getVersion());
  if (!_response.getCode()) {
    if (_request.getMethod() != METHOD_POST || !_route->upload)
      setResponseStatus(200);
    else
      setResponseStatus(201);
  }
  _response.setResource(_resource);
  _keepalive =
      (_response.getVersion() == "HTTP/1.1" && _response.getCode() != 400 &&
       (!_request.hasHeader("Connection") ||
        _request.getHeaders().find("Connection")->second == "keep-alive"));
  setResponseHeaders();
  // Ensure that Response is valid
  if (!_response.isReady())
    throw std::logic_error("Incomplete response generated");
}

void Session::setResponseHeaders() {
  WsLog::_(LVL_INFO, TGT_SESS, "Setting Response headers");
  Headers headers;

  // Server
  headers.insert("Server", "webserv");

  // Date
  std::string date = getDate();
  if (!date.empty())
    headers.insert("Date", date);

  // Content-Type / Content-Length
  if (_response.hasBody()) {
    if (!dynamic_cast<StaticResource *>(_resource))
      headers.insert("Content-Type", getMimeType("resource.html"));
    else
      headers.insert("Content-Type", getMimeType(_resourcePath));
    headers.insert("Content-Length", toString(_resource->stream().size()));
  }

  // Connection
  if (_keepalive)
    headers.insert("Connection", "keep-alive");
  else
    headers.insert("Connection", "close");

  // Last-Modified
  if (_response.getCode() == 200 && _next != DOCGI) {
    struct stat statbuf;

    if (stat(_resourcePath.c_str(), &statbuf) == 0) {
      std::string lmDate = getDate(statbuf.st_mtim.tv_sec);
      if (!lmDate.empty())
        headers.insert("Last-Modified", lmDate);
    }
  }

  // Location
  if (_response.getCode() == 301) {
    std::string location(_route->redirect);
    if (!_route->redirect.empty()) {
      location = _route->redirect;
      if (_request.getURL().size() > _route->path.size())
        location =
            joinPaths(location, _request.getURL().substr(_route->path.size()));
    } else
      location = _request.getURL() + '/';
    location = encodeURI(location);
    if (_request.hasQuery())
      location = location + '?' + _request.getQuery();
    headers.insert("Location", location);
  } else if (_response.getCode() == 201) {
    std::string location(_route->path);
    location = joinPaths(location, _route->upload_dir);
    location =
        joinPaths(location, _request.getURL().substr(_route->path.size()));
    location = normalizeURI(location);
    headers.insert("Location", encodeURI(location));
  }

  // Allow
  if (_response.getCode() == 405) {
    std::vector<std::string> allowed(_route->methods.size());
    std::transform(_route->methods.begin(), _route->methods.end(),
                   allowed.begin(), methodToString);
    headers.insert("Allow", join(allowed));
  }
  // ...

  _response.addHeaders(headers.begin(), headers.end());
}

void Session::setResponseStatus(Response::StatusCode code) {
  _response.setCode(code);
  _response.setReason(getStatusReason(code));
}
