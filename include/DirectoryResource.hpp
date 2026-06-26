/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DirectoryResource.hpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/26 07:10:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/26 07:17:25 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"

class DirectoryResource: public Resource {
public:
	DirectoryResource(const std::string &dirpath);
	DirectoryResource(const DirectoryResource &);
	DirectoryResource &operator=(const DirectoryResource &);
	~DirectoryResource();

	virtual void generate();
	virtual bool done() const;
	virtual bool inProgress() const;
	virtual bool failed() const;
	virtual const std::string &getContent() const;
private:
};
