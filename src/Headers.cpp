/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Headers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 12:45:09 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/24 14:09:34 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Headers.hpp"
#include "helpers.hpp"
#include <sstream>

// Methods
bool Headers::has(const std::string &key) const {
  return find(key) != _container.end();
}

void Headers::insert(std::string key, const std::string value) {
  capitalize(key);
  _container.insert(Header(key, value));
}

void Headers::insert(Header header) {
  capitalize(header.first);
  _container.insert(header);
}

void Headers::remove(const std::string key) { _container.erase(key); }

Headers::const_iterator Headers::find(const std::string key) const {
  return _container.find(capitalize(key));
}

std::string Headers::str() const {
  std::stringstream oss;
  for (const_iterator it = begin(), ite = end(); it != ite; it++)
    oss << it->first << ": " << it->second << '\n';
  return oss.str();
}

void Headers::clear() { _container.clear(); }

HeadersContainer::size_type Headers::size() const { return _container.size(); }
