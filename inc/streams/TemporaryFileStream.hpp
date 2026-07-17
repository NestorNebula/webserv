/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TemporaryFileStream.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 11:54:19 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/04 12:49:13 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Stream.hpp"

class TemporaryFileStream : public Stream {
public:
  TemporaryFileStream();
  virtual ~TemporaryFileStream();

private:
  TemporaryFileStream(const TemporaryFileStream &);
  TemporaryFileStream &operator=(const TemporaryFileStream &);

  const static std::string::size_type _maxPathSize = 20;
  char _path[_maxPathSize];

  void openTmpFile();
  std::string getNextFilePath();
};
