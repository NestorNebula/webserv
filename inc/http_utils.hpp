/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_utils.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 13:08:43 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/05 14:21:56 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Response.hpp"
#include "ServerConfig.hpp" // Maxime's header

bool isAllowedMethod(HttpMethod method, RouteConfig &config);

bool isValidVersion(const std::string &version);

RouteConfig *findBestRoute(const std::string &url, ServerConfig &config);

std::string resolvePath(const std::string &url, RouteConfig &config);

std::string joinPaths(const std::string &prefix, const std::string &suffix);

bool isExistingFile(const std::string &path);

bool isDirectory(const std::string &path);

bool isCgi(const std::string &path, RouteConfig &config);

bool isAccessibleFile(const std::string &path);

std::string getStatusReason(Response::StatusCode code);
