/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Stream.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 10:46:38 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 11:44:33 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <Stream.hpp>

void Stream::adoptStream(std::iostream *stream) {
	delete _stream;
	_stream = stream;
}

Stream::streamsize Stream::gcount() const {
	throwIfNull();
	return _stream->gcount();
}

Stream &Stream::getline(char *s, streamsize n, char delim) {
	throwIfNull();
	_stream->getline(s, n, delim);
	return *this;
}

Stream &Stream::read(char *s, streamsize n) {
	throwIfNull();
	_stream->read(s, n);
	return *this;
}

Stream &Stream::seekg(streampos pos) {
	throwIfNull();
	_stream->seekg(pos);
	return *this;
}

Stream &Stream::seekg(streamoff off, ios_base::seekdir way) {
	throwIfNull();
	_stream->seekg(off, way);
	return *this;
}

// ostream methods
Stream &Stream::write(const char *s, streamsize n) {
	throwIfNull();
	_stream->write(s, n);
	return *this;
}

Stream &Stream::seekp(streampos pos) {
	throwIfNull();
	_stream->seekp(pos);
	return *this;
}

Stream &Stream::seekp(streamoff off, ios_base::seekdir way) {
	throwIfNull();
	_stream->seekp(off, way);
	return *this;
}

Stream &Stream::flush() {
	throwIfNull();
	_stream->flush();
	return *this;
}

// ios methods
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

Stream::operator void*() const {
	throwIfNull();
	return *_stream;
}

Stream::streambuf *Stream::rdbuf() const {
	throwIfNull();
	return _stream->rdbuf();
}

Stream::streambuf *Stream::rdbuf(streambuf *sb) {
	throwIfNull();
	return _stream->rdbuf(sb);
}

void Stream::throwIfNull() const {
	if (!_stream)
		throw std::logic_error("calling method on null Stream pointer");
}
