/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Stream.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 09:53:43 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/01 12:33:17 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "WsLog.hpp"
#include <iostream>

class Stream {
public:
  Stream() : _stream(NULL), _g(0), _p(0), _gcount(0) {
    WsLog::_(LVL_DBG, TGT_STRM, "Stream constructor");
  }
  Stream(std::iostream *stream, std::streampos g = 0, std::streampos p = 0) : _stream(stream), _g(g), _p(p), _gcount(0) {}
  virtual ~Stream() {
    WsLog::_(LVL_DBG, TGT_STRM, "Stream destructor");
    delete _stream;
  }

  typedef std::streamsize streamsize;
  typedef std::streampos streampos;
  typedef std::streamoff streamoff;
  typedef std::ios_base ios_base;
  typedef std::streambuf streambuf;

  typedef enum eSeekDir {
    BEG = ios_base::beg,
    CUR = ios_base::cur,
    END = ios_base::end,
  } SeekDir;

  // Stream becomes the owner of the given stream
  void adoptStream(std::iostream *stream, streampos g = 0, streampos p = 0);

  // istream methods
  streamsize gcount() const;
  Stream &getline(char *s, streamsize n, char delim = '\n');
  Stream &read(char *s, streamsize n);
  streampos tellg();
  Stream &seekg(streampos pos);
  Stream &seekg(streamoff off, SeekDir way);

  // ostream methods
  Stream &write(const char *s, streamsize n);
  streampos tellp();
  Stream &seekp(streampos pos);
  Stream &seekp(streamoff off, SeekDir way);
  Stream &flush();

  // ios methods
  bool good() const;
  bool eof() const;
  bool fail() const;
  bool bad() const;
  bool operator!() const;
  operator void *() const;

  // Custom methods
  streamsize size();
  Stream &read(std::string &s);

  // Create a string from Stream content.
  // Should only be used on small-sized Streams or for testing.
  std::string str();

  template <typename T> Stream &operator>>(T &t) {
    throwIfNull();
    _stream->seekg(_g);
    *_stream >> t;
    _g = _stream->tellg();
    return *this;
  }

  template <typename T> Stream &operator<<(T const &t) {
    throwIfNull();
    _stream->seekp(_p);
    *_stream << t;
    _p = _stream->tellp();
    return *this;
  }

protected:
  std::iostream *_stream;
  void throwIfNull() const;

private:
  Stream(const Stream &);
  Stream &operator=(const Stream &);

  streampos _g;
  streampos _p;
  streamsize _gcount;
};
