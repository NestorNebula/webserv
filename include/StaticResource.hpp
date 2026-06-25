/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticResource.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 09:42:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/25 09:47:02 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"

class StaticResource: public Resource {
public:
	StaticResource(const std::string &path);
	virtual void generate();
	virtual bool done() const;
	virtual bool inProgress() const;
	virtual bool failed() const;
	virtual const std::string &getContent() const;

private:
	StaticResource(const StaticResource &);
	StaticResource &operator=(const StaticResource &);
};
