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
	Stream(): _stream(NULL) {
		WsLog::_(LVL_DBG, TGT_STRM, "Stream constructor");
	}
	Stream(std::iostream *stream): _stream(stream) {}
	~Stream() { 
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
	void adoptStream(std::iostream *stream);

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
	operator void*() const;
	streambuf *rdbuf() const;
	streambuf *rdbuf(streambuf *sb);

	template <typename T>
	Stream &operator>>(T &t) {
		throwIfNull();
		*_stream >> t;
		return *this;
	}

	template <typename T>
	Stream &operator<<(T const &t) {
		throwIfNull();
		*_stream << t;
		return *this;
	}

protected:
	std::iostream *_stream;
	void throwIfNull() const;

private:
	Stream(const Stream &);
	Stream &operator=(const Stream &);
};
