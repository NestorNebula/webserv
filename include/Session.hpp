/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 14:56:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/06/30 16:01:03 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Resource.hpp"
#include "Stream.hpp"

class Session {
public:
	Session();
	~Session();

	// Action to do from Network
	typedef enum eAction {
		RDSOCK, // Read from Connection socket
		DOCGI, // Handle CGI
		WRSOCK, // Write to Connection socket
		CLOSE, // Close the Connection
	} Action;

	Action nextAction() const;

	// Write data to the Session Request. Corresponds to RDSOCK Action.
	Stream::streamsize write(char *buf, Stream::streamsize count);

	// Give access to the data of an executed CGI script. Corresponds to DOCGI Action.
	void setCgiResource(Resource *cgiResource);

	// Read data from the Session Response. Corresponds to WRSOCK Action.
	Stream::streamsize read(char *buf, Stream::streamsize bufsize);

private:
	Session(const Session &);
	Session &operator=(const Session &);
};
