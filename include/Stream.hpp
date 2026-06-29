/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Stream.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 09:53:43 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 13:52:50 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>

class Stream {
public:
	Stream(): _stream(NULL) {}
	Stream(std::iostream *stream): _stream(stream) {}
	~Stream() { delete _stream; }

	typedef std::streamsize streamsize;
	typedef std::streampos streampos;
	typedef std::streamoff streamoff;
	typedef std::ios_base ios_base;
	typedef std::streambuf streambuf;

	// Stream becomes the owner of the given stream
	void adoptStream(std::iostream *stream);

	// istream methods
	Stream &getline(char *s, streamsize n, char delim = '\n');
	Stream &read(char *s, streamsize n);
	Stream &seekg(streampos pos);
	Stream &seekg(streamoff off, ios_base::seekdir way);

	// ostream methods
	Stream &write(const char *s, streamsize n);
	Stream &seekp(streampos pos);
	Stream &seekp(streamoff off, ios_base::seekdir way);
	Stream &flush();

	// ios methods
	bool good() const;
	bool eof() const;
	bool fail() const;
	bool bad() const;
	bool operator!() const;
	operator void*() const;
	streambuf *rdbuf() const;
	streambuf *rdbuf(streambuf *sb);

	template <typename T>
	Stream &operator>>(T &t) {
		throwIfNull();
		_stream >> t;
		return *this;
	}

	template <typename T>
	Stream &operator<<(T const &t) {
		throwIfNull();
		_stream << t;
		return *this;
	}

protected:
	std::iostream *_stream;
	void throwIfNull() const;

private:
	Stream(const Stream &);
	Stream &operator=(const Stream &);
};
