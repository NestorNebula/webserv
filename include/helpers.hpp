/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 10:25:26 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/21 11:41:09 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

std::string trim(std::string s, std::string set);

void capitalize(std::string &s);
std::string capitalize(const std::string &s);
