/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TemporaryFileStream.cpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/04 11:12:28 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/04 12:48:39 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "TemporaryFileStream.hpp"
#include <fstream>
#include <unistd.h>

TemporaryFileStream::TemporaryFileStream() {
	std::tmpnam(_path);
	std::fstream *fstream = new std::fstream();
	fstream->open(_path, std::fstream::in | std::fstream::out | std::fstream::binary);
	if (!fstream->is_open())
		throw std::runtime_error("failed to open temporary file");
	_stream = fstream;
}

TemporaryFileStream::~TemporaryFileStream() {
	static_cast<std::fstream *>(_stream)->close();
	std::remove(_path);
}

void TemporaryFileStream::openTmpFile() {
	while (_stream == NULL) {
		std::string tmpFilePath = getNextFilePath();
		std::fstream *fstream = new std::fstream();
		fstream->open(tmpFilePath.c_str(), std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
		if (fstream->is_open())
			_stream = fstream;
		else
			delete fstream;
	}
}

std::string TemporaryFileStream::getNextFilePath() {
	const static std::string safeChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
	const static std::string path(".tmp/");
	static std::string filename = safeChars.substr(0, 1);

	while (access(std::string(path + filename).c_str(), F_OK) == 0) {
		for (std::string::reverse_iterator it = filename.rbegin(), ite = filename.rend(); it != ite; it++) {
			std::string::size_type charIndex = safeChars.find(*it);
			if (charIndex == safeChars.size() - 1) {
				*it = safeChars[0];
				if (it + 1 == ite) {
					filename.push_back(safeChars[0]);
					break;
				}
			}
			else {
				*it = safeChars[charIndex + 1];
				break;
			}
		}
		if (filename.size() + path.size() >= _maxPathSize)
			filename = safeChars[0];
	}
	return path + filename;
}
