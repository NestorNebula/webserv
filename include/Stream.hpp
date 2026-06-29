/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Stream.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 09:53:43 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 10:30:09 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>

using std::streamsize, std::streampos, std::streamoff, std::ios_base, std::streambuf;

class Stream {
public:
	Stream();
	~Stream();

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
	streambuf *rdbuf(streambuf *sb) const;

private:
	Stream(const Stream &);
	Stream &operator=(const Stream &);
};

template <typename T>
Stream &operator>>(Stream &stream, T const &t);

template <typename T>
Stream &operator<<(Stream &stream, T const &t);
