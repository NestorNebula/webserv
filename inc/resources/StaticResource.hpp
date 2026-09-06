/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticResource.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 09:42:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/09/06 23:03:36 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"
#include "WsLog.hpp"

class StaticResource : public Resource {
public:
  StaticResource(const std::string &filepath)
      : _filepath(filepath), _state(DEFAULT), _stream(NULL) {
    WSLOG(LVL_DBG, TGT_STAT_RES,
             "StaticResource constructor for: ", filepath);
  }
  virtual ~StaticResource() {
    WSLOG(LVL_DBG, TGT_STAT_RES,
             "StaticResource destructor for: ", _filepath);
    delete _stream;
  }
  virtual void generate();
  virtual bool done() const { return _state == DONE; }
  virtual bool inProgress() const { return false; }
  virtual bool failed() const { return _state == FAIL; }
  virtual Stream &stream();

protected:
  typedef enum eInternalState {
    DEFAULT,
    DONE,
    FAIL,
  } InternalState;

  StaticResource(const StaticResource &);
  StaticResource &operator=(const StaticResource &);

  std::string _filepath;
  InternalState _state;
  Stream *_stream;
};

// #kd - Default Error String
class ErrorResource : public StaticResource {
public:
  ErrorResource(const std::string &filepath, const std::string &def_str) : StaticResource(filepath), _def_str(def_str)  {
    WSLOG(LVL_DBG, TGT_STAT_RES,
             "ErroResource constructor for: ", filepath);
    }
  ~ErrorResource() {}
  void generate()
  {
    StaticResource::generate();
    if (!_stream || _state != DONE)
    {
      if (_stream)
        delete (_stream);
      std::stringstream * sstr = new std::stringstream(_def_str);
      _stream = new Stream(sstr);
      _state = DONE;
      WSCOL(WSL_CYAN);
      WSLOG(LVL_ERR, TGT_STAT_RES, "using : ErrorResource::def_str");
    }
  }
private:
  const std::string & _def_str;
};