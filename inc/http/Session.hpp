/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 14:56:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/08/21 11:44:56 by kdonlon          ###   ########.fr       */
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
    delete _resource;
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

  // Set an error code for the current Session and prepare the appropriate
  // content. Callable whatever the current action is. Keep in mind that it will
  // erase the current Session Resource in case action is already WRSOCK.
  void setError(Response::StatusCode code);

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


// #kd
public:
  std::string resp;
  std::string &get_resp(void)
  {
    if (resp.size())
      return (resp);
    char buf[RSP_READ_SIZ];
    int err = this->read(buf, RSP_READ_SIZ);
    if (err > 0)
      resp.append(buf, err);
    return (resp);
  }
  void  log_next(void)
  {
    WSCOL(WSL_YELLOW);
    switch(this->nextAction())
    {
    case Session::RDSOCK:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  RDSOCK");
      break;
    case Session::DOCGI:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  DOCGI");
      break;
    case Session::WRSOCK:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  WRSOCK");
      break;
    case Session::CLOSE:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  CLOSE");
      break;
    case Session::KPALIVE:
      WSLOG(LVL_DBG, TGT_CONN_SEND, "next:  KPALIVE");
      break;
    }
  }

};
