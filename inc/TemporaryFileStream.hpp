/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TemporaryFileStream.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 11:54:19 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 13:01:51 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Stream.hpp"
#include <fstream>

class TemporaryFileStream: public Stream {
public:
	TemporaryFileStream() {
		std::tmpnam(_path);
		std::fstream *fstream = new std::fstream();
		fstream->open(_path, std::fstream::in | std::fstream::out | std::fstream::binary);
		if (!fstream->is_open())
			throw std::runtime_error("failed to open temporary file");
		_stream = fstream;
	}
	~TemporaryFileStream() {
		static_cast<std::fstream *>(_stream)->close();
		std::remove(_path);
	}

private:
	TemporaryFileStream(const TemporaryFileStream &);
	TemporaryFileStream &operator=(const TemporaryFileStream &);

	char _path[L_tmpnam];
};
