/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RouteConfig.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:56:10 by mamarti           #+#    #+#             */
/*   Updated: 2026/09/04 22:42:07 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTECONFIG_HPP
# define ROUTECONFIG_HPP

# include "HttpMethod.hpp"

# include <vector>
# include <map>


struct RouteConfig {
	std::string					path;
	std::string					root;
	bool						autoindex;
	bool						upload;
	std::string					upload_dir;
	size_t						max_body_size;
	std::set<HttpMethod>		methods;
	std::vector<std::string>	index;
	std::string					redirect;
// #kd - we no longer need this
	// std::string					pycgi_dir;

	std::map<std::string, std::string>	error_pages;
	std::map<std::string, std::string>	cgi;

	RouteConfig();
};

#endif
