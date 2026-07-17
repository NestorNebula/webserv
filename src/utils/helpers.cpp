/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   helpers.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 10:27:00 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/04 14:32:45 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "helpers.hpp"
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <ctime>

std::string trim(std::string s, std::string set, bool beg, bool end) {
  if (s.empty())
    return s;
  std::string::size_type startIndex, endIndex;
  startIndex = beg ? s.find_first_not_of(set) : 0;
  endIndex = end ? s.find_last_not_of(set) : s.size() - 1;
  if (startIndex == std::string::npos || endIndex == std::string::npos)
    return std::string();
  return std::string(s, startIndex, endIndex - startIndex + 1);
}

void capitalize(std::string &s) {
  for (std::string::iterator it = s.begin(), ite = s.end(); it != ite; it++) {
    if (it == s.begin() || !std::isalpha(*(it - 1)))
      *it = std::toupper(static_cast<unsigned char>(*it));
    else
      *it = std::tolower(static_cast<unsigned char>(*it));
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

std::vector<std::string> split(const std::string &s, const std::string set) {
  std::vector<std::string> splitStr;
  std::string tmp;

  for (std::string::const_iterator it = s.begin(), ite = s.end(); it != ite;
       it++) {
    if (set.find(*it) == std::string::npos)
      tmp.push_back(*it);
    else if (!tmp.empty()) {
      splitStr.push_back(tmp);
      tmp.clear();
    }
  }
  if (!tmp.empty())
    splitStr.push_back(tmp);
  return splitStr;
}

std::string getDate() {
  std::time_t time = std::time(NULL);
  return (getDate(time));
}

std::string getDate(std::time_t time) {
  std::tm *tm = std::gmtime(&time);
  if (tm == NULL)
    return std::string();
  return getDate(tm);
}
std::string getDate(std::tm *tm) {
  if (tm == NULL)
    return std::string();
  char buf[30];
  std::strftime(buf, 30, "%a, %d %b %Y %H:%M:%S GMT", tm);
  return std::string(buf);
}
