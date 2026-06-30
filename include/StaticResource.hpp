/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticResource.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/25 09:42:42 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/25 14:02:21 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"

class StaticResource: public Resource {
public:
	StaticResource(const std::string &filepath): _filepath(filepath), _state(DEFAULT), 
	_stream(NULL) {}
	~StaticResource() {
		delete _stream;
	}
	virtual void generate();
	virtual bool done() const { return _state == DONE; }
	virtual bool inProgress() const { return false; }
	virtual bool failed() const { return _state == FAIL; }
	virtual Stream &stream();

private:
	typedef enum eInternalState {
		DEFAULT,
		DONE,
		FAIL,
	} InternalState;

	StaticResource(const StaticResource &);
	StaticResource &operator=(const StaticResource &);

	std::string _filepath;
	InternalState _state;
	Stream *_stream;
};
