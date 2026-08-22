/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FilePath.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 18:40:20 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/22 12:50:19 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILE_PATH_HPP
# define FILE_PATH_HPP

# include <iostream>
# include <string>
# include <deque>

class FilePath
{
public:
    std::string     praw;
    std::string     path;
    std::string     fldr;
    std::string     file;
    std::string     fext;
    
    FilePath() {}
    void parse(std::string & p)
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
    void dump(void)
    {
        std::cerr << "praw : " << praw << std::endl;
        std::cerr << "path : " << path << std::endl;
        std::cerr << "fldr : " << fldr << std::endl;
        std::cerr << "file : " << file << std::endl;
        std::cerr << "fext : " << fext << std::endl;
    }
};


#endif