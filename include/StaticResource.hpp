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
#include <fstream>

class StaticResource: public Resource {
public:
	StaticResource(const std::string &filepath): _filepath(filepath), _state(DEFAULT), _file(filepath.c_str()) {}
	~StaticResource() {
		if (_file.is_open())
			_file.close();
	}
	virtual void generate();
	virtual bool done() const { return _state == DONE; }
	virtual bool inProgress() const { return false; }
	virtual bool failed() const { return _state == FAIL; }
	virtual const std::string &getContent() const;

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
	std::string _content;
	std::ifstream _file;

	bool readContent();
};
