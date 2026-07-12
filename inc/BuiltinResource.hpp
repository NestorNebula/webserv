/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BuiltinResource.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/12 08:50:02 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 10:02:30 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"
#include "Response.hpp"

class BuiltinResource : public Resource {
public:
  BuiltinResource(Response::StatusCode code)
      : _code(code), _state(DEFAULT), _stream(NULL) {}
  ~BuiltinResource() { delete _stream; }

  virtual void generate();
  virtual bool done() const { return _state == DONE; }
  virtual bool inProgress() const { return false; }
  virtual bool failed() const { return _state == FAIL; }
  virtual Stream &stream();

private:
  typedef enum eInternalState {
    DEFAULT,
    DONE,
    FAIL,
  } InternalState;

  BuiltinResource(const BuiltinResource &);
  BuiltinResource &operator=(const BuiltinResource &);

  Response::StatusCode _code;
  InternalState _state;
  Stream *_stream;

  void buildHTMLFromCode();
};
