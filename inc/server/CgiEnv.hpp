/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:46:53 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/18 15:59:29 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_ENV_HPP
# define CGI_ENV_HPP

# include <string>
# include <map>
# include <vector>
# include <sstream>

# include "FilePath.hpp"
# include "Session.hpp"

// Utils.hpp
template <typename T>
std::string num_2_str(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

class Connection;

enum
{
	CGI_PHP = 1,
	CGI_PYTHON,
	CGI_PERL
};

class CgiEnv
{
private:
	CgiEnv				(const CgiEnv & );
	CgiEnv & operator = (const CgiEnv & ) { return (*this); }
	
public:
	CgiEnv (void);
	~CgiEnv();
	
    int             from_conn(Connection & conn);
	void		    add(const char *key, const char *val);
	void		    add(const char *key, int n);
	std::string &	get(const char *key);
	const char	    **gen(void);
	
	const char							*args[4];
	int									lang;
	std::map<std::string, std::string>	kv;
	
private:
	std::vector<std::string>			data;
	const char							**res;
	
	Session::CgiInfo					info; // hm : internal to session (?) or a type
	std::string							exec;
	FilePath							script;
};

#endif


#if 0 

Changes to be committed:
(use "git restore --staged <file>..." to unstage)
modified:   inc/http/Headers.hpp
modified:   inc/http/Request.hpp
modified:   inc/http/Session.hpp
modified:   src/http/Request.cpp
modified:   src/http/Response.cpp
modified:   src/http/Session.cpp

Changes not staged for commit:
(use "git add <file>..." to update what will be committed)
(use "git restore <file>..." to discard changes in working directory)
modified:   inc/http/Request.hpp
modified:   inc/http/Session.hpp
modified:   inc/server/CgiEnv.hpp
modified:   src/server/CgiEnv.cpp



#endif
