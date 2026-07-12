/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BuiltinResource.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/12 08:50:02 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/12 08:54:41 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"
#include "Response.hpp"

class BuiltinResource: public Resource {
public:
	BuiltinResource(Response::StatusCode code);
	~BuiltinResource();
	
	virtual void generate();
	virtual bool done() const;
	virtual bool inProgress() const;
	virtual bool failed() const;
	virtual Stream &stream();

private:
	BuiltinResource(const BuiltinResource &);
	BuiltinResource &operator=(const BuiltinResource &);
};
