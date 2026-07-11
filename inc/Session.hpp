/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nhoussie <nhoussie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 14:56:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/07/05 14:19:24 by nhoussie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ServerConfig.hpp" // Maxime's Header
#include "Request.hpp"
#include "Resource.hpp"
#include "Response.hpp"
#include "Stream.hpp"
#include "WsLog.hpp"

class Session {
public:
	Session(ServerConfig &server): _next(RDSOCK), _server(server), _route(NULL), _resource(NULL), _keepalive(false), _sent(0) {
		WsLog::_(LVL_DBG, TGT_SESS, "Session constructor");
	}
	~Session() {
		WsLog::_(LVL_DBG, TGT_SESS, "Session destructor");
	}

	// Action to do from Network
	typedef enum eAction {
		RDSOCK, // Read from Connection socket
		DOCGI, // Handle CGI
		WRSOCK, // Write to Connection socket
		CLOSE, // Close the Connection
		KPALIVE, // Keep the Connection alive
	} Action;

	Action nextAction() const { return _next; }

	// Write data to the Session Request. Corresponds to RDSOCK Action.
	Stream::streamsize write(const char *buf, Stream::streamsize count);

	// Give access to the data of an executed CGI script. Corresponds to DOCGI Action.
	void setCgiResource(Resource *cgiResource);

	// Read data from the Session Response. Corresponds to WRSOCK Action.
	Stream::streamsize read(char *buf, Stream::streamsize bufsize);

	// Reset session state and clears all its data
	void reset();

private:
	Session(const Session &);
	Session &operator=(const Session &);

	Action _next;

	ServerConfig &_server;
	RouteConfig *_route;
	std::string _resourcePath;

	Request _request;
	Resource *_resource;
	Response _response;

	bool _keepalive;

	void throwIfNotAction(Action action) const;
	void manageSession();

	void handleRequest();
	void preValidateRequest();
	void validateRequest();
	void resolveResource();
	void validateOperation();

	void handleResource();
	void prepareErrorResource();
	void prepareDirectoryResource();

	void handleUpload();
	void handleDelete();

	void handleResponse();
	void setResponseHeaders();
	void setResponseStatus(Response::StatusCode code);

	Stream::streamsize _sent;
};
