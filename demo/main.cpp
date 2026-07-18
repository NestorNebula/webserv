#include "ConfigParser.hpp"
#include "Epoll.hpp"
#include "Server.hpp"
#include "Session.hpp"
#include "WsLog.hpp"
#include "unistd.h"
#include <fstream>
#include <iostream>

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

#define COLOR_DEF "\x1b[0m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"

static bool setWorkingDirectory(const std::string &path);

int main(int argc, char *argv[], char **envp) {
  WsLog::tgt = TGT_CONN_SEND; // TGT_MAX >> 1;
  WsLog::lvl = LVL_ALL;
  if (argc < 2) {
    std::cerr << "usage: demo <config>\n";
    return 0;
  }

  if (!setWorkingDirectory(argv[1])) { // Replace with config path in real main
    std::cerr << "couldn't setup working directory.\n";
    return 0;
  }
  ConfigParser parser;
  parser.parseFile(argv[1]);
  const std::vector<ServerConfig> &servers = parser.getServers();

  Epoll epoll(envp);
  for (std::vector<ServerConfig>::const_iterator it = servers.begin(),
                                                 ite = servers.end();
       it != ite; it++)
    new Server(&epoll, it->port, *it);
  epoll.loop();

  return 0;
}

static bool setWorkingDirectory(const std::string &path) {
  std::string::size_type lastSlash = path.find_last_of('/');
  if (lastSlash == std::string::npos)
    return true;
  std::string directory = path.substr(0, lastSlash);
  return chdir(directory.c_str()) == 0;
}
