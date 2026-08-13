/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpMethod.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 12:07:34 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/03 12:08:43 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPMETHOD_HPP
# define HTTPMETHOD_HPP

# include <string>
# include <set>

enum	HttpMethod {
	METHOD_GET,
	METHOD_POST,
	METHOD_DELETE,
	METHOD_UNKNOWN	// for invalid or unknown method
};

HttpMethod	stringToMethod(const std::string& string);
std::string	methodToString(HttpMethod method);

std::set<HttpMethod>	parseMethods(const std::string& value);

#endif
