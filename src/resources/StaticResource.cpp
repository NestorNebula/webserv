/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticResource.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 11:24:31 by nhoussie          #+#    #+#             */
/*   Updated: 2026/09/01 18:43:10 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "StaticResource.hpp"
#include <fstream>
#include <stdexcept>

void StaticResource::generate() {
  if (_state != DEFAULT)
    throw std::logic_error("generate called multiple times");

  WSLOG(LVL_INFO, TGT_STAT_RES, "Opening ", _filepath);
  std::fstream *fs = new std::fstream(_filepath.c_str());
  _stream = new Stream(fs);
  _state = fs->is_open() ? DONE : FAIL;
  if (_state == FAIL) {
    WSCOL(WSL_PURPLE);
    WSLOG(LVL_TMP, TGT_STAT_RES, "Failed to open ", _filepath);
  }
}

Stream &StaticResource::stream() {
  if (!_stream || _state != DONE)
    throw std::logic_error("stream not accessible");
  return *_stream;
}
