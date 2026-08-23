/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FilePath.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 18:40:20 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 15:28:16 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILE_PATH_HPP
# define FILE_PATH_HPP

# include <unistd.h>
# include <iostream>
# include <string>
# include <deque>

bool         setWorkingDirectory(const std::string &path, std::string &cwd);
std::string  getConfigFileName(const std::string &path);
int          env_pwd(char **envp, std::string &str);


class FilePath
{
public:
    std::string     praw;
    std::string     path;
    std::string     fldr;
    std::string     file;
    std::string     fext;
    
    FilePath() {}
    FilePath(std::string &str) { this->parse(str); }
    ~FilePath() {}
    
    void    parse(std::string & p);
    void    dump(void);
};


#endif