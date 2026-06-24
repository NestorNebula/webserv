/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/24 11:08:55 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/24 14:05:01 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Headers.hpp"
#include "Resource.hpp"

class Response {
public:
  typedef unsigned int StatusCode;

  Response(Resource &resource);
  ~Response();

  const std::string &getVersion() const;
  void setVersion(std::string version);
  StatusCode getCode() const;
  void setCode(StatusCode code);
  const std::string &getReason() const;
  void setReason(std::string reason);
  const Headers &getHeaders() const;
  void addHeader(Header header);
  void addHeaders(Headers::const_iterator begin, Headers::const_iterator end);
  const Resource &getResource() const;
  const std::string &getRaw() const; // or char *getRaw() const;

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
};
