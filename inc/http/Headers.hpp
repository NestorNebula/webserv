/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Headers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:29:47 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/25 14:21:24 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <map>
#include <string>

typedef std::pair<std::string, std::string> Header;

class Headers {
public:
  typedef std::map<std::string, std::string> Container;
  typedef Container::size_type size_type;

  typedef Container::iterator iterator;
  typedef Container::const_iterator const_iterator;
  typedef Container::reverse_iterator reverse_iterator;
  typedef Container::const_reverse_iterator const_reverse_iterator;

  // Methods
  bool has(const std::string &key) const;
  void insert(std::string key, const std::string value);
  void insert(Header header);
  void remove(const std::string key);
  const_iterator find(const std::string key) const;
  std::string str(const std::string eol = "\r\n") const;
  void clear() { _container.clear(); }
  size_type size() const { return _container.size(); }
  std::string get(const std::string &key) const;

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
  Container _container;
};
