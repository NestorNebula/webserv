/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 10:27:00 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/21 15:13:17 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.hpp"
#include <cctype>
#include <cerrno>
#include <cstdlib>

std::string trim(std::string s, std::string set) {
  std::string::size_type start, end;
  start = s.find_first_not_of(set);
  end = s.find_last_not_of(set);
  if (start == std::string::npos || end == std::string::npos)
    return s;
  return std::string(s, start, end - start + 1);
}

void capitalize(std::string &s) {
  for (std::string::iterator it = s.begin(), ite = s.end(); it != ite; it++) {
    if (it == s.begin() || !std::isalpha(*(it - 1)))
      *it = std::toupper(*it);
  }
}

std::string capitalize(const std::string &s) {
  std::string copy(s);
  capitalize(copy);
  return copy;
}

long getLong(const std::string &s, bool *err, long min, long max, int base,
             char endc) {
  char *endptr = NULL;
  errno = 0;
  long n = std::strtol(s.c_str(), &endptr, base);
  if (err)
    *err = true;
  if (*endptr != endc ||
      ((n == LONG_MIN || n == LONG_MAX) && errno == ERANGE) ||
      (n < min || n > max))
    return n;
  if (err)
    *err = false;
  return n;
}
