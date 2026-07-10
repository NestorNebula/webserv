/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/01 08:32:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/05 14:37:50 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Session.hpp"
#include "DirectoryResource.hpp"
#include "Request.hpp"
#include "StaticResource.hpp"
#include "HttpMethod.hpp"
#include "http_utils.hpp"
#include <cstring>
#include <fstream>
#include <sstream>

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

	_next = RDSOCK;
}

void Session::throwIfNotAction(Action action) const {
	if (action != _next)
		throw std::logic_error("Wrong action on Session");
}

void Session::manageSession() {
	switch (_next) {
		case RDSOCK:
			if (_request.isComplete() || _request.isInvalid()) {
				handleRequest();
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
		case CLOSE: case KPALIVE:
			if (_response.getVersion() == "HTTP/1.1")
				_next = KPALIVE;
			else
				_next = CLOSE;
			break;
		default:
			break;
	}
}

void Session::handleRequest() {
	validateRequest();
	resolveResource();
	validateOperation();
}

void Session::validateRequest() {
	if (_request.hasHeaders() && _request.getHeaders().str().size() > MAX_HEADERS_SIZE)
		return setResponseStatus(431);
	if (_request.hasBody() && static_cast<unsigned int>(_request.getBody()->size()) > _server.max_body_size)
		return setResponseStatus(413);
	// TODO Manage pre- end of request tests
	if (_request.isInvalid() || !isValidVersion(_request.getVersion())
			|| (_request.getVersion() == "HTTP/1.1" && !_request.getHeaders().has("Host")))
		return setResponseStatus(400);
	if (_request.getVersion() != "HTTP/1.0" && _request.getVersion() != "HTTP/1.1")
		return setResponseStatus(505);
	if (_request.getMethod() == METHOD_UNKNOWN) // TODO move check up
		return setResponseStatus(501);
}

void Session::resolveResource() {
	if (_response.getCode()) return;
	_route = findBestRoute(_request.getURL(), _server);
	if (!_route)
		return setResponseStatus(404);
	if (!_route->redirect.empty())
		return setResponseStatus(301);
	if (!isAllowedMethod(_request.getMethod(), *_route))
		return setResponseStatus(405);
	
	_resourcePath = resolvePath(_request.getURL(), *_route);
	if (isCgi(_resourcePath, *_route)) {
		_next = DOCGI;
		return;
	}
}

void Session::validateOperation() {
	if (_response.getCode()) return;
	if (_next == DOCGI) {
		if (!isAccessibleFile(_resourcePath))
			return setResponseStatus(403);
		return;
	}
	if (_request.getMethod() == METHOD_POST && _route->upload) {
		if (isExistingFile(_resourcePath))
			return setResponseStatus(403);
		return;
	}
	if (!isExistingFile(_resourcePath))
		return setResponseStatus(404);
	if (!isAccessibleFile(_resourcePath))
		return setResponseStatus(403);
}

void Session::handleResource() {
	if (_next == DOCGI)
		return;
	if (_response.getCode() >= 400)
		prepareErrorResource();
	else if (!_response.getCode()) {
		switch (_request.getMethod()) {
			case METHOD_GET:
				if (isDirectory(_resourcePath))
					prepareDirectoryResource();
				else
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
	// Generate Resource
	if (_resource) {
		_resource->generate();
		// Handle Resource errors
		if (_resource->failed()) {
			setResponseStatus(500);
			delete _resource;
			_resource = NULL;
		}
	}
}

void Session::prepareErrorResource() {
	std::map<std::string, std::string> errPages = !_route ? _server.error_pages : _route->error_pages;
	std::ostringstream oss;
	oss << _response.getCode();
	std::string errPage = joinPaths(_server.root, errPages.find(oss.str()) != errPages.end() ? errPages[oss.str()] : errPages["default"]);
	if (!isAccessibleFile(errPage))
		errPage = joinPaths(_server.root, errPages["default"]);
	delete _resource;
	_resource = new StaticResource(errPage);
}

void Session::prepareDirectoryResource() {
	bool indexFound = false;
	for (std::vector<std::string>::const_iterator it = _route->index.begin(), ite = _route->index.end(); it != ite && !indexFound; it++) {
		std::string indexPath = joinPaths(_route->root, *it);
		if (isAccessibleFile(indexPath)) {
			_resourcePath = indexPath;
			indexFound = true;
		}
	}
	if (indexFound)
		_resource = new StaticResource(_resourcePath);
	else if (_route->autoindex)
		_resource = new DirectoryResource(_resourcePath);
	else {
		setResponseStatus(403);
		prepareErrorResource();
	}
}

void Session::handleUpload() {
	std::string uploadDir = joinPaths(_route->root, _route->upload_dir);
	if (!isDirectory(uploadDir))
		return setResponseStatus(400);
	std::string uploadFile = joinPaths(uploadDir, _request.getURL().substr(_route->path.size()));
	if (isExistingFile(uploadFile))
		return setResponseStatus(403);
	std::ofstream ofs(uploadFile.c_str());
	if (!ofs.is_open())
		return setResponseStatus(500);
	Stream *bodyStream = _request.hasBody() ? _request.getBody() : NULL;
	if (bodyStream && *bodyStream) {
		char buf[BUFSIZE];
		while (bodyStream->read(buf, BUFSIZE))
			ofs.write(buf, bodyStream->gcount());
		if (bodyStream->eof() && bodyStream->gcount())
			ofs.write(buf, bodyStream->gcount());
	}
	ofs.close();
	if ((bodyStream && !bodyStream->eof()) || !ofs) {
			std::remove(uploadFile.c_str());
			return setResponseStatus(500);
	}
	setResponseStatus(201);
}

void Session::handleDelete() {
	if (isDirectory(_resourcePath))
		return setResponseStatus(403);
	errno = 0;
	if (std::remove(_resourcePath.c_str()) && errno == EACCES)
		return setResponseStatus(403);
	setResponseStatus(204);
}

void Session::handleResponse() {
	// Add Response details and missing fields
	if (!isValidVersion(_request.getVersion()))
		_response.setVersion("HTTP/1.1");
	else
		_response.setVersion(_request.getVersion());
	if (!_response.getCode()) {
		if (_request.getMethod() != METHOD_POST || !_route->upload)
			setResponseStatus(200);
		else
			setResponseStatus(201);
	}
	setResponseHeaders();
	_response.setResource(_resource);
	// Ensure that Response is valid
	if (!_response.isReady())
		throw std::logic_error("Incomplete response generated");
}

void Session::setResponseHeaders() {
	// TODO
	// Add missing headers
	Headers headers;

	headers.insert("Server", "Webserv");
	// Date header
	// Content-Length header
	// Content-Type header
	// ...
	
	_response.addHeaders(headers.begin(), headers.end());
}

void Session::setResponseStatus(Response::StatusCode code) {
	_response.setCode(code);
	_response.setReason(getStatusReason(code));
}
