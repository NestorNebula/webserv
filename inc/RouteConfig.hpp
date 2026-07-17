/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RouteConfig.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 11:56:10 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 13:42:22 by mamarti          ###   ########.fr       */
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

	std::map<std::string, std::string>	error_pages;
	std::map<std::string, std::string>	cgi;

	RouteConfig();
};

#endif
