/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FcgiMsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 19:37:08 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 17:45:07 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FcgiMsg.hpp"

FcgiMsgData::FcgiMsgData()
{
	this->zero();
}

void FcgiMsgData::zero()
{
	siz  = 0;
	len  = 0;
	req  = 0;
	role = 0;
	typ  = 0;
	pad  = 0;
}


#if 0


https://fastcgi-archives.github.io/FastCGI_Specification.html

    The Responder application receives CGI/1.1 environment variables from the Web server over FCGI_PARAMS.

    Next the Responder application receives CGI/1.1 stdin data from the Web server over FCGI_STDIN. The application receives at most CONTENT_LENGTH bytes from this stream before receiving the end-of-stream indication. (The application receives less than CONTENT_LENGTH bytes only if the HTTP client fails to provide them, e.g. because the client crashed.)

    The Responder application sends CGI/1.1 stdout data to the Web server over FCGI_STDOUT, and CGI/1.1 stderr data over FCGI_STDERR. The application sends these concurrently, not one after the other. The application must wait to finish reading FCGI_PARAMS before it begins writing FCGI_STDOUT and FCGI_STDERR, but it need not finish reading from FCGI_STDIN before it begins writing these two streams.

    After sending all its stdout and stderr data, the Responder application sends a FCGI_END_REQUEST record. The application sets the protocolStatus component to FCGI_REQUEST_COMPLETE and the appStatus component to the status code that the CGI program would have returned via the exit system call.

A Responder performing an update, e.g. implementing a POST method, should compare the number of bytes received on FCGI_STDIN with CONTENT_LENGTH and abort the update if the two numbers are not equal.

SO : send the WHOLE request (headers included)


#endif

FcgiMsg::FcgiMsg()
{
	this->zero();
}


void FcgiMsg::new_params(unsigned short req)
{
	buf.clear();

	this->begin(); // (BEGIN_REQUEST, RESPONDER)
	set_req(req);  // head.requestId

	buf.push(&head, FCGI_HEADER_LEN);
	buf.push(&body, FCGI_HEADER_LEN); // body.role

	pHed = buf.end; // remember where to insert FCGI_PARAMS
	buf.skip(FCGI_HEADER_LEN);
	pBeg = buf.end; // for calculating length of FCGI_PARAMS
}

const char * p_name[] =
{
	"SERVER_PROTOCOL",
	"REQUEST_METHOD",
	"SCRIPT_FILENAME",
	"CONTENT_TYPE",
	"QUERY_STRING",
	"CONTENT_LENGTH",
	"HTTP_ACCEPT",
	"MIME_TYPE",
	"SERVER_ADDR",
	"REQUEST_URI",
	"SERVER_PORT",
	"HTTP_COOKIE",
	"HTTP_HOST",
	"HTTPS",
	"DOCUMENT_ROOT",
	"REMOTE_ADDR"
	// "SCRIPT_NAME",
	// "REDIRECT_STATUS",
	// "HTTP_REFERER",
	// "HTTP_USER_AGENT"
	// "HTTP_TRANSFER_ENCODING"
	// "HTTP_ACCEPT_ENCODING"
	// "HTTP_ACCEPT_LANGUAGE"
	// "HTTP_CONNECTION"
	// "SERVER_NAME"
	// "SERVER_SOFTWARE"
	// "GATEWAY_INTERFACE"  "CGI/1.0"
};

// buf.fcgi() : keylen, vallen, key, val
// push fcgi-formatted key/val pair onto buf
// kLen, vLen, kStr, vStr
void FcgiMsg::add_param(const char * key, const char * val)
{
	buf.fcgi(key, val);
}

void FcgiMsg::add_param(const char * key, int val)
{
	std::string vStr = num_2_str(val);
	buf.fcgi(key, vStr.c_str());
}

void FcgiMsg::end_params(void)
{
	this->make_head(FCGI_PARAMS, buf.end - pBeg);

	// insert params header with proper content-length

	ft_memcpy(buf.buf + pHed, this, FCGI_HEADER_LEN);
	buf.zero(this->head.paddingLength);

	this->make_head(FCGI_PARAMS, 0);
	buf.push(this, FCGI_HEADER_LEN);
}

void FcgiMsg::add_stdin(const char * data, int dSiz)
{
	const int dMax = 8192 << 1;
	while (dSiz > dMax)
	{
		this->make_head(FCGI_STDIN, dMax);
		buf.push(this, FCGI_HEADER_LEN);
		buf.push(data, dMax);

		data += dMax;
		dSiz -= dMax;
	}
	if (dSiz)
	{
		this->make_head(FCGI_STDIN, dSiz);
		buf.push(this, FCGI_HEADER_LEN);
		buf.push(data, dSiz);
		buf.zero(this->head.paddingLength);
		// padding (?)
		// buf.zero()
	}
}

void FcgiMsg::end_stdin(void)
{
	WSCOL(WSL_RED);
	WSLOG(LVL_DBG, TGT_FCGI, "END STDIN");
	this->make_head(FCGI_STDIN, 0);
	buf.push(this, FCGI_HEADER_LEN);
}


void FcgiMsg::begin()
{
	this->zero();
	head.type = FCGI_BEGIN_REQUEST;
	set_len(FCGI_HEADER_LEN); // FCGI_BEGIN_REQUEST (body)
	set_role(FCGI_RESPONDER);
}

void FcgiMsg::make_head(unsigned char typ, unsigned short len)
{
	this->zero();
	head.type = typ;
	set_len(len);
	set_pad(len); // could be in (set_len)
}

int FcgiMsg::full_size()
{
	return FCGI_HEADER_LEN + this->get_len() + this->head.paddingLength;
}



void FcgiMsg::set_pad(unsigned short len)
{
	unsigned short hi8 = (len + 7) & ~(7);
	head.paddingLength = hi8 - len;
}
void FcgiMsg::set_req(unsigned short req)
{
	head.requestIdB1 = req >> 8;
	head.requestIdB0 = req & 0xff;
}
unsigned short FcgiMsg::get_req()
{
	return (unsigned short) head.requestIdB1 << 8 | head.requestIdB0;
}

void FcgiMsg::set_len(unsigned short len)
{
	head.contentLengthB1 = len >> 8;
	head.contentLengthB0 = len & 0xff;
}
unsigned short FcgiMsg::get_len()
{
	return (unsigned short) head.contentLengthB1 << 8 | head.contentLengthB0;
}
void FcgiMsg::set_role(unsigned short role)
{
	body.roleB1 = role >> 8;
	body.roleB0 = role & 0xff;
}
unsigned short FcgiMsg::get_role()
{
	return (unsigned short) body.roleB1 << 8 | body.roleB0;
}
void FcgiMsg::zero()
{
	head.version = FCGI_VERSION_1; // 0x1
	head.type = 0;

	set_req(0);
	set_len(0);

	head.paddingLength = 0;
	head.reserved = 0;

	set_role(0);
	body.flags = 0;
	ft_memset(body.reserved, 0, 5);
}

void FcgiMsg::info()
{
    unsigned short hlen = this->get_len();
        
    std::cerr << "typ (" << (unsigned int) this->head.type << ") ";
    std::cerr << "len (" << hlen << ") ";
    std::cerr << "pad (" << (unsigned int) this->head.paddingLength << ") ";
    std::cerr << "tot (" << (hlen + this->head.paddingLength) << ")\n";
    
}

void FcgiMsg::data(FcgiMsgData * data)
{
	data->typ  = this->head.type;
	data->pad  = this->head.paddingLength;
	data->siz  = this->full_size();
	data->len  = this->get_len();
	data->req  = this->get_req();
	data->role = this->get_role();

}
