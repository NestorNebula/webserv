/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bridge.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 15:47:24 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/20 01:11:59 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "bridge.hpp"


// GET Requests: The encoded string is appended to the URL and passed to the CGI script via the QUERY_STRING environment variable. 

// POST Requests: The encoded data is sent in the request body. The script must read the number of bytes specified by the CONTENT_LENGTH environment variable from standard input (stdin). 

// Decoding: The script must parse the string and decode the URL-encoded characters to retrieve the original form values. 


#if 0 // chatgpt
std::string decodeChunked(const std::string& chunked) {
    std::string body;
    size_t pos = 0;

    while (true) {
        size_t lineEnd = chunked.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            throw std::runtime_error("Malformed chunked encoding");

        std::string lenStr = chunked.substr(pos, lineEnd - pos);
        size_t chunkSize = std::stoul(lenStr, nullptr, 16);

        pos = lineEnd + 2;

        if (chunkSize == 0)
            break;

        if (pos + chunkSize > chunked.size())
            throw std::runtime_error("Incomplete chunk");

        body.append(chunked, pos, chunkSize);

        pos += chunkSize;

        if (chunked.substr(pos, 2) != "\r\n")
            throw std::runtime_error("Missing CRLF after chunk");

        pos += 2;
    }

    return body;
}
#endif
static ssize_t	chunk_size(std::string & str, size_t off)
{
    size_t lineEnd = str.find("\r\n", off);
    if (lineEnd == std::string::npos)
        return (0);

    std::string lenStr = str.substr(off, lineEnd - off);
    ssize_t chunkSize = strtoul(lenStr.c_str(), NULL, 16);
    // off = lineEnd + 2;
    str.erase(off, lineEnd - off + 2);
    if (chunkSize == 0)
        return (0);

    // if (pos + chunkSize > chunked.size())
    //     throw std::runtime_error("Incomplete chunk");
    return (chunkSize);
    
    // unsigned int x;   
	// std::stringstream ss(str.substr(off));
	// ss >> std::hex >> x;
	// size_t pos = ss.tellg(); // end of string (!) -- what .. don't have \r\n !!!
    // WsLog::_(LVL_DBG, TGT_HEAD, "csiz: erase ", off, pos);
    // if (pos == std::string::npos)
    // {
    //     str.erase(off);
    //     return (x);
    // }
	// str.erase(off, pos + 2); // CRLF
	// if (x == 0)
	// 	str.erase(off, 2);
	// return (x);
}
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
        if (!chnk)
            return (this->state);
        csiz = chunk_size(body, 0);
        if (csiz <= 0)
            return (this->state);
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
        csiz -= body.size();      
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);

        WsLog::_(LVL_DBG, TGT_HEAD, "body: ", body.substr(0, 256));
        return (this->state);
    }
    else
    {
        this->body.append(buf, siz);
        blen += siz; // including chunk info
        
    }
    // off of previous -- will change 
    // if we are erasing from (body)
    // is chunked "nice" .. sending only the header
    // so we can approve (?)
    // so we only need to un-chunk here (?)
    
    
    WsLog::_(LVL_DBG, TGT_HEAD, "blen: ", blen);
    if (!chnk)
    {
        WsLog::_(LVL_DBG, TGT_HEAD, "clen: ", clen);
        return (this->state);
    }

    WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
    WsLog::_(LVL_DBG, TGT_HEAD, "bsiz: ", body.size());
    if (csiz > 0)
    {
        csiz -= siz;
        if (csiz <= 0)
        {
            WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
            WsLog::_(LVL_DBG, TGT_HEAD, "bsiz: ", body.size());   
        }
    }
    if (csiz <= 0)
    {
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: refresh");
        
#if 1
// MAY NEED MORE BODY DATA
        size_t end = body.size() + csiz;
        if (csiz == 0)
            end = 0;
        WsLog::_(LVL_DBG, TGT_HEAD, "end : ", end);
        
        WsLog::_(LVL_DBG, TGT_HEAD, "BODY: ", body.substr(end, 128));

        csiz = chunk_size(body, end);
        // if (csiz < 0)
            // need more 
        if (csiz)
            csiz -= (body.size() - end);
        // else
            // maybe not enough
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
        WsLog::_(LVL_DBG, TGT_HEAD, "BODY: ", body.substr(end, 128));

        // std::string crlf("\r\n");
        // size_t	chk = body.find(crlf, end);
        // if (chk != end)
        // {
        //     WsLog::_(LVL_DBG, TGT_HEAD, "end : expected crlf ", chk, end);
        //     return (this->state);
        //     // chnk = 0;
        //     // return (this->state);
        //     // as long as chk is not npos .. we should be ok
        // }
        // body.erase(chk, 2);
        // 
        // 
        //     // should have been erased .. 
        // WsLog::_(LVL_DBG, TGT_HEAD, "BODY: ", body.substr(chk, 128));
        // csiz -= (body.size() - chk);
        // have not yet got \r\n following -- not part of body size 
        // so .. two bytes short
        
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
#endif
        // if (chk + csiz < (ssize_t) body.size())
        //     WsLog::_(LVL_DBG, TGT_HEAD, "CSIZ: check again");
        // chnk = 0;
#if 0
        ssize_t beg = body.size() - siz;
        if (beg < 0)
            beg = 0;
        WsLog::_(LVL_DBG, TGT_HEAD, "beg : ", beg);
        
        std::string crlf("\r\n");
        
        // could be finding eol of DATA (!)
        size_t	eol = body.find(crlf, beg);
        if (eol == std::string::npos)
        {
            WsLog::_(LVL_DBG, TGT_HEAD, "eol : not found");
        }
        // is eol end of DATA (?)
        csiz = chunk_size(body, beg);
        // and .. subtract what we just received 
        WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
        chnk = 0;
        // size_t beg = body.rfind(crlf, pos);
        // if (beg == std::string::npos)
        // {
        //     WsLog::_(LVL_DBG, TGT_HEAD, "beg : not found");
        //     beg = 0;
        // }
        // csiz = chunk_size(body, beg);
        // // and .. subtract what we just received 
        // WsLog::_(LVL_DBG, TGT_HEAD, "csiz: ", csiz);
        // WsLog::_(LVL_DBG, TGT_HEAD, "body: ", body.substr(beg, 128));
        // // blen = body.size()
#endif
    }
    // something not clean here 
    if (csiz < 0)
    {
        size_t end = body.size() + csiz;
        WsLog::_(LVL_DBG, TGT_HEAD, "END : ", body.substr(end, 128));
    }
    
    return (this->state);
}


void Request::clear(void)
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
    
	WsLog::_(LVL_DBG, TGT_HEAD, "head: ", head);
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
        {
            chnk = 1;
        }
    }
	WsLog::_(LVL_DBG, TGT_HEAD, "chnk: ", chnk);
    return (0);
}

int Request::body_stat(void)
{
    if (this->body.size())
        return (1);
    if (this->chnk)
    {
        if (csiz < 0)
            return (-1);
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
