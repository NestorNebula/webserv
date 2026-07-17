/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 07:10:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/26 12:03:29 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"
#include "WsLog.hpp"
#include <dirent.h>

class DirectoryResource : public Resource {
public:
  DirectoryResource(const std::string &dirpath)
      : _dirpath(dirpath), _state(DEFAULT), _dir(opendir(dirpath.c_str())),
        _stream(NULL) {
    WsLog::_(LVL_DBG, TGT_DIR_RES,
             "DirectoryResource constructor for: ", dirpath);
  }
  ~DirectoryResource() {
    WsLog::_(LVL_DBG, TGT_DIR_RES,
             "DirectoryResource destructor for: ", _dirpath);
    if (_dir)
      closedir(_dir);
    delete _stream;
  }

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

  DirectoryResource(const DirectoryResource &);
  DirectoryResource &operator=(const DirectoryResource &);

  std::string _dirpath;
  InternalState _state;
  DIR *_dir;
  Stream *_stream;

  void buildList();
};
