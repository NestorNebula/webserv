/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:29 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/14 16:38:44 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BRIDGE_HPP
# define BRIDGE_HPP

# include <string>
# include <CgiPipe.hpp>

# define REQ_INIT 0
# define REQ_READ_HEAD 1
# define REQ_HAVE_HEAD 2
# define REQ_READ_BODY 3
# define REQ_HAVE_BODY 4
# define REQ_DONE 5

class Request
{
public:
    Request(void) : state(REQ_INIT) {}
    
    std::string head;
    std::string body;
    std::string exec;
    
    int push_data(const char *buf, size_t siz)
    {
        if (this->state < REQ_HAVE_HEAD)
        {
            this->head.append(buf, siz);

            std::string hed_end("\r\n\r\n");
            size_t	crlf = head.find(hed_end);
            if (crlf == std::string::npos)
                return (this->state);
                
            this->state = REQ_HAVE_HEAD;
            body = head.substr(crlf + 4);
            head.erase(crlf + 4);
            return (this->state);
        }
        
        this->body.append(buf, siz);
        // body.size() == content-length
        return (this->state);
    }
    
    std::string header(const char *key);

    int get_state(void) { return this->state; }
private:
    int state;
};

class Resource
{
public:
	pid_t			cgi_pid;
	CgiPipe			*cgi_ip;
	CgiPipe			*cgi_op;
};

class Session
{
public:
    Session(void) {}
    Request req;
  
    int push_data(const char *buf, size_t siz)
    {
        int err = this->req.push_data(buf, siz);
        if (err < REQ_HAVE_HEAD)
            return (err);
        // have head : make rsrc
        // conn->exec_cgi()
        
        return (err);
    }
    int req_state(void) { return this->req.get_state(); }
};

#endif