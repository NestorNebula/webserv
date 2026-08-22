/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FilePath.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/22 15:23:02 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/22 15:26:41 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FilePath.hpp"

bool setWorkingDirectory(const std::string &path, std::string &cwd) 
{
    std::string::size_type lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos)
        return true;
    std::string directory = path.substr(0, lastSlash);
    cwd += directory + std::string("/");
    return chdir(directory.c_str()) == 0;
}

std::string getConfigFileName(const std::string &path) 
{
    std::string::size_type lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos)
        return path;
    return path.substr(lastSlash + 1);
}


int  env_pwd(char **envp, std::string &str)
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



void FilePath::parse(std::string & p)
{   
    this->praw = p;
    this->path.clear();
    this->fldr.clear();
    this->file.clear();
    this->fext.clear();
    
    size_t b = 0;
    size_t e = 0;

    std::deque<std::string> segs;
    while (1)
    {
        e = p.find("/", b);
        if (e == std::string::npos)
        {
            this->file = p.substr(b);

            b = this->file.find_last_of(".");
            if (b != std::string::npos)
                this->fext = this->file.substr(b);

                    
            std::deque<std::string>::iterator it = segs.begin();
            while (it != segs.end())
            {
                this->path += *it++;
                this->path += "/";
            }
            this->fldr  = this->path;
            this->path += this->file;
            break;
        }
        std::string chk = p.substr(b, e - b);
        if (chk == std::string("."))
            ;
        else if (chk == std::string(".."))
            segs.pop_back();
        else
            segs.push_back(chk);
        b = e + 1;
    }
}
void FilePath::dump(void)
{
    std::cerr << "praw : " << praw << std::endl;
    std::cerr << "path : " << path << std::endl;
    std::cerr << "fldr : " << fldr << std::endl;
    std::cerr << "file : " << file << std::endl;
    std::cerr << "fext : " << fext << std::endl;
}
