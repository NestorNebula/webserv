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

#include "Stream.hpp"

class Resource {
public:
	virtual ~Resource() = 0;

	virtual void generate() = 0;
	virtual bool done() const = 0;
	virtual bool inProgress() const = 0;
	virtual bool failed() const = 0;
	virtual Stream &stream() = 0;
};

inline Resource::~Resource() {}
