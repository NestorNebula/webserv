/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiEnv.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/07 19:46:53 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/12 19:56:19 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_ENV_HPP
# define CGI_ENV_HPP

# include <string>
# include <vector>
# include <sstream>

// Utils.hpp
template <typename T>
std::string num_2_str(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

class Connection;

class CgiEnv
{
private:
	CgiEnv (const CgiEnv & that);
	CgiEnv & operator = (const CgiEnv & )
		{ return (*this); }
public:
	CgiEnv (void);
	~CgiEnv();
	
    int             from_conn(Connection & conn, std::string & file);
	void		    add(const char *key, const char *val);
	void		    add(const char *key, int n);
	const char	    **gen(void);
	
private:
	std::vector<std::string>	data;
	const char					**res;
};

#endif