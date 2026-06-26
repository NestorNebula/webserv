/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 11:59:05 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/26 12:05:04 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DirectoryResource.hpp"
#include <stdexcept>

void DirectoryResource::generate() {}

const std::string &DirectoryResource::getContent() const {
	if (_state != DONE)
		throw std::logic_error("Content not available");
	return _content;
}
