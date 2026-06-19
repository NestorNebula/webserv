/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Headers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 12:45:09 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/19 13:20:15 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Headers.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

// Methods
bool Headers::has(const std::string &key) const {
	return find(key) != _container.end();
}

void Headers::insert(const std::string &key, const std::string &value) {
	std::transform(key.begin(), key.end(), key.begin(), tolower);
	_container.insert(Header(key, value));
}

void Headers::remove(const std::string &key) {
	_container.erase(key);
}

Headers::const_iterator Headers::find(const std::string &key) const {
	return _container.find(key);
}

std::string Headers::str() const {
	std::stringstream oss;
	for (const_iterator it = begin(), ite = end(); it != ite; it++)
		oss << it->first << ": " << it->second << '\n';
	return oss.str();
}

void Headers::clear() {
	_container.clear();
}
