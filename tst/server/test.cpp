/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/16 18:48:00 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WsLog.hpp"

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"
#include "ConfigParser.hpp"
#include "FilePath.hpp"

#include <deque>

static bool         setWorkingDirectory(const std::string &path, std::string &cwd);
static std::string  getConfigFileName(const std::string &path);

static int          env_pwd(char **envp, std::string &str);

int main (int argc, char ** argv, char **envp)
{   

    // std::string p("/media/kdonlon/data/Documents/42/webserv/git/tst/server/../../demo/./../tst/server/www/test.php");

    // FilePath chk(p);
    
    // return (0);

    WsLog::kd();
    // WsLog::nh();

    if (argc < 2)
    {
        std::cerr << "usage: demo <config>\n";
        return 0;
    }
    if (argc > 2)
    {
        switch(argv[2][0])
        {
        case '0':
           WsLog::tgt = TGT_NONE;
           break;
        case 'k':
           WsLog::tgt = TGT_KD;
           break;
        case 'a':
           WsLog::tgt = TGT_ALL; //  & ~(TGT_CGI_HEAD | TGT_CGI_DATA);
           break;
        }
    }
    
    std::string conf_root;
    if (env_pwd(envp, conf_root))
    {
        std::cerr << "couldn't detect working directory\n";
        return (0);
    }
    if (!setWorkingDirectory(argv[1], conf_root)) 
    {
        std::cerr << "couldn't setup working directory.\n";
        return 0;
    }
    
    ConfigParser parser(conf_root);
    try 
    {
        parser.parseFile(getConfigFileName(argv[1]));
    } 
    catch (std::exception &e) 
    {
        std::cout << e.what() << "\n";
        return 0;
    }

    const std::vector<ServerConfig> &servers = parser.getServers();



    int     err = 0;
    Epoll   *ep = NULL;
    
    try
    {
        ep = new Epoll(envp); // , servers)
       
        err = ep->serve(servers);

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


static bool setWorkingDirectory(const std::string &path, std::string &cwd) 
{
    std::string::size_type lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos)
        return true;
    std::string directory = path.substr(0, lastSlash);
    cwd += directory + std::string("/");
    return chdir(directory.c_str()) == 0;
}

static std::string getConfigFileName(const std::string &path) 
{
    std::string::size_type lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos)
        return path;
    return path.substr(lastSlash + 1);
}


static int  env_pwd(char **envp, std::string &str)
{
    char **chk = envp;
    while (*chk)
    {
        if (std::string(*chk).substr(0,4) == std::string("PWD="))
        {
            str = std::string(*chk).substr(4) + std::string("/");
            return (0);
        }
        chk++;
    }
    return (1);
}
