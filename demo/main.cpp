#include "ConfigParser.hpp"
#include "Epoll.hpp"
#include "Server.hpp"
#include "unistd.h"
#include <iostream>

static bool       setWorkingDirectory(const std::string &path);

static std::string getConfigFileName(const std::string &path);

int main(int argc, char *argv[], char **envp) {
  WsLog::tgt = TGT_ALL; // (TGT_SERV | TGT_HTTP);
  WsLog::lvl = LVL_ALL;
  if (argc < 2) {
    std::cerr << "usage: demo <config>\n";
    return 0;
  }

  if (!setWorkingDirectory(argv[1])) {
    std::cerr << "couldn't setup working directory.\n";
    return 0;
  }
  ConfigParser parser;
  try {
	parser.parseFile(getConfigFileName(argv[1]));
  } catch (std::exception &e) {
	  std::cout << e.what() << "\n";
	  return 0;
  }

  
  const std::vector<ServerConfig> &servers = parser.getServers();


    int     err = 0;
    Epoll   *ep = NULL;
    
    try
    {
        ep = new Epoll(envp); // , servers)
       
        std::vector<ServerConfig>::const_iterator it = servers.begin();
        std::vector<ServerConfig>::const_iterator ite = servers.end();
        for ( ;it != ite; it++)
        {
            try
            {
                new Server(ep, it->port, *it);
                err = 1;
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        } 
        if (err)
          err = ep->loop();
        WsLog::_(LVL_INFO, TGT_MAIN, "exit: ", err);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


    delete (ep);
    return (err);
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
