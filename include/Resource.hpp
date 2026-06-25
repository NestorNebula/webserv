/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Resource.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/24 11:16:46 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/24 15:47:10 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

class Resource {
public:
	virtual ~Resource() = 0;

	virtual const std::string &getContent() const = 0; // or char *getContent() const;
};

inline Resource::~Resource() {}
