/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:56:01 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 12:26:33 by mamarti          ###   ########.fr       */
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
	std::string					upload_dir;
	std::set<HttpMethod>		methods;
	std::vector<std::string>	index;

	std::map<std::string, std::string>	error_pages;
	std::vector<RouteConfig>	routes;

	ServerConfig();
};

#endif
