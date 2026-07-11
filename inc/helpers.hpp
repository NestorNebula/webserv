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

#include <climits>
#include <sstream>
#include <string>
#include <vector>

std::string trim(std::string s, std::string set, bool beg = true, bool end = true);

void capitalize(std::string &s);
std::string capitalize(const std::string &s);

long getLong(const std::string &s, bool *err, long min = LONG_MIN,
             long max = LONG_MAX, int base = 10, char endc = '\0');

std::vector<std::string> split(const std::string &s, const std::string set);

template <typename T>
std::string toString(T t) {
	std::ostringstream oss;
	oss << t;
	return oss.str();
}
