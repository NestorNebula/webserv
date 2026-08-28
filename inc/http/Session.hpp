/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 14:56:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/08/28 11:06:43 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include "Resource.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp"
#include "Stream.hpp"
#include "WsLog.hpp"
#include "SizeDefs.hpp"

class Session {
public:
  Session(ServerConfig &server)
      : _next(RDSOCK), _server(server), _route(NULL), _resource(NULL),
        _keepalive(false), _sent(0) {
    WSLOG(LVL_DBG, TGT_SESS, "Session constructor");
  }
  ~Session() {
    try
    {
      /* code */
      delete _resource;
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
    
    WSLOG(LVL_DBG, TGT_SESS, "Session destructor");
  }

  // Action to do from Network
  typedef enum eAction {
    RDSOCK,  // Read from Connection socket
    DOCGI,   // Handle CGI
    WRSOCK,  // Write to Connection socket
    CLOSE,   // Close the Connection
    KPALIVE, // Keep the Connection alive
  } Action;

  Action nextAction() const { return _next; }

  // Write data to the Session Request. Corresponds to RDSOCK Action.
  // Can be used for DOCGI action while Request isn't complete or invalid
  Stream::streamsize write(const char *buf, Stream::streamsize count);

  struct CgiInfo {
    std::string scriptPath;
    std::string executablePath;
  };

  // Give access to the Session Request. Should only be called on DOCGI action.
  Request &getRequest();

  // Give informations about the CGI to be executed. Should only be called on
  // DOCGI action.
  Session::CgiInfo getCgiInfo() const;

  // Give access to the data of an executed CGI script. Corresponds to DOCGI
  // Action.
  void setCgiResource(Resource *cgiResource);

  // Read data from the Session Response. Corresponds to WRSOCK Action.
  Stream::streamsize read(char *buf, Stream::streamsize bufsize);

  // Read data from the Session Response and append it to the Response string.
  // Return a reference to the Response string.
  // Should only be called on WRSOCK action.
  std::string &getResponse();

  // Set an error code for the current Session and prepare the appropriate
  // content. Callable whatever the current action is. Keep in mind that it will
  // erase the current Session Resource in case action is already WRSOCK.
  void setError(Response::StatusCode code);

  // Reset session state and clears all its data
  void reset();
// #kd - set keep-alive 
  Action _next;
private:
  Session(const Session &);
  Session &operator=(const Session &);

  // Action _next;

  ServerConfig &_server;
  RouteConfig *_route;
  std::string _resourcePath;

  Request _request;
  Resource *_resource;
  Response _response;

  bool _keepalive;

  void throwIfNotAction(Action action) const;
  static const std::string &actionToStr(Action action);
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

  std::string _responseStr;
};
