/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Headers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:29:47 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/21 11:03:45 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <map>
#include <string>

typedef std::string Key;
typedef std::string Value;
typedef std::pair<Key, Value> Header;
typedef std::map<Key, Value> HeadersContainer;

class Headers {
public:
  // Canonical Form
  Headers() {}
  Headers(const Headers &headers) : _container(headers._container) {}
  Headers &operator=(const Headers &headers) {
    if (this != &headers)
      _container = headers._container;
    return *this;
  }
  ~Headers() {}

  typedef HeadersContainer::iterator iterator;
  typedef HeadersContainer::const_iterator const_iterator;
  typedef HeadersContainer::reverse_iterator reverse_iterator;
  typedef HeadersContainer::const_reverse_iterator const_reverse_iterator;

  // Methods
  bool has(const std::string &key) const;
  void insert(std::string &key, const std::string &value);
  void insert(Header &header);
  void remove(const std::string &key);
  const_iterator find(const std::string &key) const;
  std::string str() const;
  void clear();
  HeadersContainer::size_type size() const;

  // Iterators
  iterator begin() { return _container.begin(); }
  iterator end() { return _container.end(); }
  reverse_iterator rbegin() { return _container.rbegin(); }
  reverse_iterator rend() { return _container.rend(); }
  const_iterator begin() const { return _container.begin(); }
  const_iterator end() const { return _container.end(); }
  const_reverse_iterator rbegin() const { return _container.rbegin(); }
  const_reverse_iterator rend() const { return _container.rend(); }

private:
  HeadersContainer _container;
};
