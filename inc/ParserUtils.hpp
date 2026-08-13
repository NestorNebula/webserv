/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserUtils.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamarti <mamarti@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 12:49:40 by mamarti           #+#    #+#             */
/*   Updated: 2026/07/10 12:32:54 by mamarti          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>

std::vector<std::string>	splitList(const std::string& value);

std::string	trim(const std::string& str);

bool	parseOnOff(const std::string& value);

#endif
