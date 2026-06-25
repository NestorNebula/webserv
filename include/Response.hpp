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

class Response {
public:
  typedef unsigned int StatusCode;

  Response(Resource &resource): _code(0), _resource(resource), _generated(false) {}

  const std::string &getVersion() const { return _version; }
  void setVersion(std::string version);
  StatusCode getCode() const { return _code; }
  void setCode(StatusCode code);
  const std::string &getReason() const { return _reason; }
  void setReason(std::string reason);
  const Headers &getHeaders() const { return _headers; }
  void addHeader(Header header);
  void addHeaders(Headers::const_iterator begin, Headers::const_iterator end);
  const Resource &getResource() const { return _resource; }

  // or char *getRaw() const;
  const std::string &getRaw() const { return _raw; }

  bool isReady() const;
  void generate();

private:
  Response(const Response &);
  Response &operator=(const Response &);

  std::string _version;
  StatusCode _code;
  std::string _reason;
  Headers _headers;
  Resource &_resource;
  std::string _raw; // or char *_raw;

  bool _generated;
  void throwIfGenerated() const;
};
