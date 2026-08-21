/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Stream.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 10:46:38 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/01 12:40:42 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Stream.hpp"
#include <algorithm>

void Stream::adoptStream(std::iostream *stream, streampos g, streampos p) {
  WsLog::_(LVL_DBG, TGT_STRM, "Stream adopting new stream");
  delete _stream;
  _stream = stream;
  _g = g;
  _p = p;
  _gcount = 0;
}

Stream::streamsize Stream::gcount() const {
  throwIfNull();
  return _gcount;
}

Stream &Stream::getline(char *s, streamsize n, char delim) {
  throwIfNull();
  _stream->seekg(_g);
  _stream->getline(s, n, delim);
  _gcount = _stream->gcount();
  _g += _gcount;
  return *this;
}

Stream &Stream::read(char *s, streamsize n) {
  throwIfNull();
  _stream->seekg(_g);
  _stream->read(s, n);
  _gcount = _stream->gcount();
  _g += _gcount;
  return *this;
}

Stream::streamsize Stream::readsome(char *s, streamsize n) {
	throwIfNull();
	_stream->seekg(_g);
	_gcount = _stream->readsome(s, n);
	_g += _gcount;
	return _gcount;
}

Stream::streampos Stream::tellg() {
  throwIfNull();
  return _g;
}

Stream &Stream::seekg(streampos pos) {
  throwIfNull();
  _stream->seekg(pos);
  _g = _stream->tellg();
  return *this;
}

Stream &Stream::seekg(streamoff off, SeekDir way) {
  throwIfNull();
  _stream->seekg(off, static_cast<std::iostream::seekdir>(way));
  _g = _stream->tellg();
  return *this;
}

Stream &Stream::write(const char *s, streamsize n) {
  throwIfNull();
  _stream->seekp(_p);
  _stream->write(s, n);
  _p = _stream->tellp();
  return *this;
}

Stream::streampos Stream::tellp() {
  throwIfNull();
  return _p;
}

Stream &Stream::seekp(streampos pos) {
  throwIfNull();
  _stream->seekp(pos);
  _p = _stream->tellp();
  return *this;
}

Stream &Stream::seekp(streamoff off, SeekDir way) {
  throwIfNull();
  _stream->seekp(off, static_cast<std::iostream::seekdir>(way));
  _p = _stream->tellp();
  return *this;
}

Stream &Stream::flush() {
  throwIfNull();
  _stream->flush();
  return *this;
}

bool Stream::good() const {
  throwIfNull();
  return _stream->good();
}

bool Stream::eof() const {
  throwIfNull();
  return _stream->eof();
}

bool Stream::fail() const {
  throwIfNull();
  return _stream->fail();
}

bool Stream::bad() const {
  throwIfNull();
  return _stream->bad();
}

bool Stream::operator!() const {
  throwIfNull();
  return !*_stream;
}

Stream::operator void *() const {
  throwIfNull();
  return *_stream;
}

Stream::streamsize Stream::size() {
  streampos curr = tellg();
  streampos streamSize;

  seekg(0, END);
  streamSize = tellg();
  seekg(curr);
  return streamSize;
}

Stream &Stream::read(std::string &s) {
  static const streamsize maxReadSize = 4096;
  const streamsize available = this->size() - this->tellg();
  if (available <= 0)
    return *this;

  const streamsize readSize = std::min(available, maxReadSize);
  char buf[maxReadSize];
  this->read(buf, readSize);
  s.append(buf, this->gcount());
  return *this;
}

std::string Stream::str() {
  WsLog::_(LVL_INFO, TGT_STRM, "calling str() method on Stream");
  if (!*this)
    return std::string();
  std::string s;
  streampos g = _g, p = _p;
  streamsize gcount = _gcount;

  seekg(0);
  for (std::string::size_type prev = s.size(); read(s) && s.size() != prev; prev = s.size());

  _g = g;
  _p = p;
  _gcount = gcount;
  if (!*this)
    _stream->clear();
  seekg(g);
  return s;
}

void Stream::throwIfNull() const {
  if (!_stream)
    throw std::logic_error("calling method on null Stream pointer");
}
