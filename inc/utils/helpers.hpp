/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 10:25:26 by nhoussie          #+#    #+#             */
/*   Updated: 2026/08/21 16:44:31 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <climits>
#include <ctime>
#include <sstream>
#include <string>
#include <vector>

std::string trim(std::string s, std::string set, bool beg = true,
                 bool end = true);

void header_key(std::string &s);

void capitalize(std::string &s);
std::string capitalize(const std::string &s);

long getLong(const std::string &s, bool *err, long min = LONG_MIN,
             long max = LONG_MAX, int base = 10, char endc = '\0');

std::vector<std::string> split(const std::string &s, const std::string set);

template <typename Container>
std::string join(Container container, const std::string &sep = ", ") {
  std::string joined;

  for (typename Container::const_iterator it = container.begin(),
                                          ite = container.end();
       it != ite; it++) {
    joined += *it;
    if (it + 1 != ite)
      joined += sep;
  }
  return joined;
}

template <typename T> std::string toString(T t) {
  std::ostringstream oss;
  oss << t;
  return oss.str();
}

std::string getDate();
std::string getDate(std::time_t time);
std::string getDate(std::tm *tm);
