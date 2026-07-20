#include "ConfigParser.hpp"
#include "Epoll.hpp"
#include "Server.hpp"
#include "unistd.h"
#include <iostream>

static bool setWorkingDirectory(const std::string &path);

static std::string getConfigFileName(const std::string &path);

int main(int argc, char *argv[], char **envp) {
  WsLog::tgt = (TGT_SERV | TGT_HTTP);
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
  parser.parseFile(getConfigFileName(argv[1]));
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

static std::string getConfigFileName(const std::string &path) {
	std::string::size_type lastSlash = path.find_last_of('/');
	if (lastSlash == std::string::npos)
		return path;
	return path.substr(lastSlash + 1);
}
