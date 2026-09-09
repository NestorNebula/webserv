/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:56:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/09/09 15:42:26 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include "HttpMethod.hpp"
# include "RouteConfig.hpp"

# include <string>
# include <vector>
# include <map>
# include <set>
# include <exception>


struct ServerConfig {
	std::string					host;
	int							port;
	size_t						max_body_size;
	std::string					root;
	bool						upload;
	bool						autoindex;
	std::string					upload_dir;
	std::string					conf_file_root;
	std::set<HttpMethod>		methods;
	std::vector<std::string>	index;

	std::map<std::string, std::string>	error_pages;
	std::vector<RouteConfig>	routes;

	std::string pycgi_dir;
	std::string fcgi_sock;
	std::string	def_err;

	ServerConfig();
};

#endif
