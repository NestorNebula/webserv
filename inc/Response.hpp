/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/24 11:08:55 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/25 14:25:51 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"
#include "Resource.hpp"
#include "WsLog.hpp"

class Response {
public:
  typedef unsigned int StatusCode;

  Response(Resource *resource = NULL): _code(0), _resource(resource) {
	  WsLog::_(LVL_DBG, TGT_RESP, "Response constructor");
  }

  const std::string &getVersion() const { return _version; }
  void setVersion(const std::string &version);
  StatusCode getCode() const { return _code; }
  void setCode(StatusCode code);
  const std::string &getReason() const { return _reason; }
  void setReason(const std::string &reason);
  const Headers &getHeaders() const { return _headers; }
  void addHeader(const Header &header);
  void addHeaders(Headers::const_iterator begin, Headers::const_iterator end);
  const Resource &getResource() const;
  void setResource(Resource *resource);

  // or char *getHead() const;
  std::string getHead() const;

  Stream::streamsize readBody(char *buf, Stream::streamsize bufsize);

  bool isReady() const;
  bool hasBody() const;
  
  void clear();

private:
  Response(const Response &);
  Response &operator=(const Response &);

  std::string _version;
  StatusCode _code;
  std::string _reason;
  Headers _headers;
  Resource *_resource;

  void throwIfNotReady() const;
};
