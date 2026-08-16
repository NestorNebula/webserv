/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/30 14:56:37 by nhoussie          #+#    #+#             */
/*   Updated: 2026/08/14 18:41:30 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include "Resource.hpp"
#include "Response.hpp"
#include "ServerConfig.hpp" // Maxime's Header
#include "Stream.hpp"
#include "WsLog.hpp"

class Session {
public:
  Session(ServerConfig &server)
      : _next(RDSOCK), _server(server), _route(NULL), _resource(NULL),
        _keepalive(false), _sent(0) {
    WsLog::_(LVL_DBG, TGT_SESS, "Session constructor");
  }
  ~Session() {
    delete _resource;
    WsLog::_(LVL_DBG, TGT_SESS, "Session destructor");
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
// #kd
  void  log_next(void)
  {
    WsLog::color(WSL_YELLOW);
    switch(this->nextAction())
    {
    case Session::RDSOCK:
      WsLog::_(LVL_DBG, TGT_CONN_SEND, "next:  RDSOCK");
      break;
    case Session::DOCGI:
      WsLog::_(LVL_DBG, TGT_CONN_SEND, "next:  DOCGI");
      break;
    case Session::WRSOCK:
      WsLog::_(LVL_DBG, TGT_CONN_SEND, "next:  WRSOCK");
      break;
    case Session::CLOSE:
      WsLog::_(LVL_DBG, TGT_CONN_SEND, "next:  CLOSE");
      break;
    case Session::KPALIVE:
      WsLog::_(LVL_DBG, TGT_CONN_SEND, "next:  KPALIVE");
      break;
    }
  }

  // Write data to the Session Request. Corresponds to RDSOCK Action.
  Stream::streamsize write(const char *buf, Stream::streamsize count);

  struct CgiInfo {
    std::string scriptPath;
    std::string executablePath;
  };

  // Give access to the Session Request. Should only be called on DOCGI action.
  // const 
  Request &getRequest();

  // Give informations about the CGI to be executed. Should only be called on
  // DOCGI action.
  Session::CgiInfo getCgiInfo() const;

  // Give access to the data of an executed CGI script. Corresponds to DOCGI
  // Action.
  void setCgiResource(Resource *cgiResource);

  // Read data from the Session Response. Corresponds to WRSOCK Action.
  Stream::streamsize read(char *buf, Stream::streamsize bufsize);

  // Reset session state and clears all its data
  void reset();
// #kd
  std::string _resourcePath;
  std::string _cgi_exec;

  std::string &get_resp(void)
  {
    if (_ostr.size())
      return (_ostr);
    char buf[4096];
    int err = this->read(buf, 4096);
    if (err)
      _ostr.append(buf, err);
    return (_ostr);
  }
private:
  std::string _ostr;

private:
  Session(const Session &);
  Session &operator=(const Session &);

  Action _next;

  ServerConfig &_server;
public: // #kd
  RouteConfig *_route;
private:
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
