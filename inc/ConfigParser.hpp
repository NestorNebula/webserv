/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 12:54:00 by mamarti           #+#    #+#             */
/*   Updated: 2026/06/22 14:03:44 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <exception>

struct RouteConfig {
	std::string	path;
	std::string	root;
	bool		autoindex;
	bool		upload;
	size_t		max_body_size;

	RouteConfig();
};

struct ServerConfig {
	std::string	host;
	int			port;
	size_t		max_body_size;
	std::string	root;
	bool		upload;
	std::string	upload_dir;

	std::map<std::string, std::string>	error_pages;
	std::vector<RouteConfig>			routes;

	ServerConfig();
};

#endif
