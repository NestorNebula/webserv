/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BuiltinResource.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/12 09:10:36 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 15:00:41 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "BuiltinResource.hpp"
#include "http_utils.hpp"
#include <sstream>

void BuiltinResource::generate() {

  if (_state != DEFAULT)
    throw std::logic_error("generate called multiple times");
  WsLog::_(LVL_INFO, TGT_BUI_RES, "Generating BuiltinResource for code ",
           _code);
  _stream = new Stream(new std::stringstream());
  buildHTMLFromCode();
  if (_state == DEFAULT)
    _state = _stream->good() ? DONE : FAIL;
  if (_state != DONE)
    WsLog::_(LVL_WARN, TGT_BUI_RES, "BuiltinResource error for code ", _code);
}

Stream &BuiltinResource::stream() {
  if (_stream == NULL || _state != DONE)
    throw std::logic_error("stream not accessible");
  return *_stream;
}

void BuiltinResource::buildHTMLFromCode() {
  std::string reason = getStatusReason(_code);
  std::ostringstream div;

  div << "<div>";
  switch (_code) {
  case 201:
    div << "The resource was created successfully.";
    break;
  case 301:
    div << "The resource has been moved permanently.";
    break;
  default:
    // Unreachable case. Small easter egg for debugging.
    _code = 418;
    reason = "I'm a teapot";
    _state = FAIL;
    WsLog::_(LVL_ERR, TGT_BUI_RES,
             "BuiltinResource called with wrong code, teapot found");
    break;
  }
  div << "</div>\n";

  *_stream << "<!DOCTYPE html>\n"
              "<html lang=\"en\">\n"
              "<head>\n"
              "<meta charset=\"utf-8\">\n"
              "<meta name=\"viewport\" content=\"width=device-width, "
              "initial-scale=1.0\" />\n"
              "<title>"
           << _code << " " << reason
           << "</title>\n"
              "</head>\n"
              "<body>\n"
              "<h1>"
           << _code << " " << reason << "</h1>\n"
           << div.str()
           << "</body>\n"
              "</html>\n";
}
