/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   TemporaryFileStream.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/29 11:54:19 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/29 11:56:05 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Stream.hpp"

class TemporaryFileStream: public Stream {
public:
	TemporaryFileStream();
	~TemporaryFileStream();

private:
	TemporaryFileStream(const TemporaryFileStream &);
	TemporaryFileStream &operator=(const TemporaryFileStream &);
};
