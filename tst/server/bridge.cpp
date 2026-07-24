/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:24 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/24 17:49:29 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bridge.hpp"


// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 

// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 

// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 

int Request::push_data(const char *buf, size_t siz)
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
        this->init();
        blen = body.size();
        return (this->state);
    }

    this->body.append(buf, siz);
    blen += siz;

    WsLog::_(LVL_DBG, TGT_BODY, "blen: ", blen);
    if (!chnk)
        WsLog::_(LVL_DBG, TGT_BODY, "clen: ", clen);
    return (this->state);
}


void Request::reset(void)
{
    head.clear();
    body.clear();
    exec.clear();

    meth.clear();
    path.clear();
    file.clear();
    fext.clear();
    vars.clear();

    blen = 0;
    clen = 0;
    chnk = 0;
    state = REQ_INIT;
}

int Request::init(void)
{
    meth.clear();
    path.clear();
    file.clear();
    fext.clear();
    vars.clear();
    
    std::stringstream	line(head);
    line >> meth >> path;
    
	size_t	pos = path.find('?');
    if (pos != std::string::npos)
    {
        vars = path.substr(pos + 1);
        path = path.substr(0, pos);
    }
    file = path;
    pos = path.rfind('/');
    if (pos != std::string::npos)
    {
        file = path.substr(pos + 1);
    }
    pos = file.rfind('.');
    if (pos != std::string::npos)
    {
        fext = file.substr(pos + 1);
    }
    
	WsLog::_(LVL_DBG, TGT_HEAD, "\n", head);
	WsLog::_(LVL_DBG, TGT_HEAD, "meth: ", meth);
	WsLog::_(LVL_DBG, TGT_HEAD, "path: ", path);
	WsLog::_(LVL_DBG, TGT_HEAD, "file: ", file);
	WsLog::_(LVL_DBG, TGT_HEAD, "fext: ", fext);
	WsLog::_(LVL_DBG, TGT_HEAD, "vars: ", vars);
    
    std::string val = header("content-length");
    if (val.size())
        clen = atoi(val.c_str());
    WsLog::_(LVL_DBG, TGT_HEAD, "clen: ", clen);

    val = header("transfer-encoding");
    if (val.size())
    {
        pos = val.find("chunked"); // case (!)
        if (pos != std::string::npos)
            chnk = 1;
    }
	WsLog::_(LVL_DBG, TGT_HEAD, "chnk: ", chnk);
    return (0);
}

int Request::body_stat(void)
{
    // WsLog::_(LVL_DBG, TGT_CONN_RECV, "body:  size ", this->body.size());
    if (this->body.size())
        return (1);
    if (this->chnk)
    {
        // check something here 
        return (0);
    }
    if (clen && blen < clen)
        return (0);
    return (-1);
}

static bool	icmp(char a, char b)
{
	return std::tolower(static_cast<unsigned char>(a)) ==
		std::tolower(static_cast<unsigned char>(b));		
}

std::string hedval_str(std::string & str, const char *key)
{
	std::string	kstr = std::string("\n") + std::string(key);
	std::string	val("");

	std::string::const_iterator it = std::search(
		str.begin(), str.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == str.end())
        return (val);
        
    std::stringstream	line(str.substr(it - str.begin()));
    line >> kstr >> val;
    return (val);
}

std::string Request::header(const char *key) const
{
	std::string	kstr(key);
	std::string	val("");
	
	if (kstr == std::string("METH"))
        return (this->meth);
	if (kstr == std::string("PATH"))
        return (this->path);
	if (kstr == std::string("FILE"))
        return (this->file);
	if (kstr == std::string("FEXT"))
        return (this->fext);
	if (kstr == std::string("VARS"))
        return (this->vars);
	
	std::string::const_iterator it = std::search(
		head.begin(), head.end(),
		kstr.begin(), kstr.end(),
		icmp);
	if (it == head.end())
	{
		// std::cerr << "header: not found\n";
		return (val);
	}
	it += strlen(key);
	if (*it != ':')
	{
		// std::cerr << "header: not colon\n";
		return (val);
	}
	it++;
	it++; // space (ugly) BUT IMPORTANT
	size_t off_beg = (it - head.begin());

	std::string hed_eol("\r\n");
	size_t	off_end = head.find(hed_eol, off_beg);

	if (off_end == std::string::npos)
	{
		return (val);
	}

	val = head.substr(off_beg, off_end - off_beg);

	std::string kv(key + std::string("=") + val);
	WsLog::_(LVL_DBG, TGT_HEAD, kv);
	return (val);
}
