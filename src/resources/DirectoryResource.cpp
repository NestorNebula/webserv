/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 11:59:05 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 10:07:44 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryResource.hpp"
#include <dirent.h>
#include <sstream>
#include <stdexcept>

void DirectoryResource::generate() {
  if (_state != DEFAULT)
    throw std::logic_error("generate called multiple times");

  WsLog::_(LVL_INFO, TGT_DIR_RES, "Directory Listing for: ", _dirpath);
  _stream = new Stream(new std::stringstream());
  if (_dir)
    buildList();
  _state = (_dir != NULL && _stream->good()) ? DONE : FAIL;
  if (_state != DONE)
    WsLog::_(LVL_WARN, TGT_DIR_RES, "Directory listing error for: ", _dirpath);
}

Stream &DirectoryResource::stream() {
  if (!_stream || _state != DONE)
    throw std::logic_error("stream not accessible");
  return *_stream;
}

void DirectoryResource::buildList() {
  *_stream << "<!DOCTYPE html>\n"
              "<html lang=\"en\">\n"
              "<head>\n"
              "<meta charset=\"utf-8\">\n"
              "<meta name=\"viewport\" content=\"width=device-width, "
              "initial-scale=1.0\" />\n"
              "<link rel=\"stylesheet\" href=\"/css/styles.css\" />"
              "<title>Directory listing for "
           << _dirpath
           << "</title>\n"
              "</head>\n"
              "<body>\n"

              "<header>\n"
              "<h1>WEBSERV</h1>\n"
              "<p>Demonstration Website</p>\n"
              "<p>for our HTTP server</p>\n"
              "</header>\n"
              "<section id=\"dir-list-section\">\n"
              "<h1>Directory listing for "
           << _dirpath
           << "</h1>\n"
              "<hr>\n"
              "<ul class=\"dir-list\">\n";

  dirent *dirFile;
  while ((dirFile = readdir(_dir)) != NULL) {
    std::string name(dirFile->d_name);
    if (name != "." && name != "..") {
      if (dirFile->d_type == DT_DIR)
        name += '/';
      *_stream << "<li>\n"
                  "<a class=\"file-link\" href=\""
               << name << "\">" << name
               << "</a>\n"
                  "</li>\n";
    }
  }
  *_stream << "</ul>\n"
              "<hr>\n"
              "</section>\n"
              "</body>\n"
              "</html>\n"
              "<footer>\n"
              "<div>© Kevin Donlon, Noa Houssier, Maxime Marti</div>\n"
              "<a "
              "href=\"https://github.com/NestorNebula/webserv\" "
              "target=\"_blank\" "
              "rel=\"noopener noreferrer\" "
              ">\n"
              "<img id=\"gh-icon\" src=\"/assets/icons/github.svg\" "
              "alt=\"GitHub\" />\n"
              "</a>\n"
              "<div>2026</div>\n"
              "</footer>\n";
}
