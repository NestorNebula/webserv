/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 05:51:27 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "Connection.hpp"

enum
{
	RSRC_RESP_INIT = 0,
	RSRC_RESP_HEAD,
	RSRC_RESP_BODY,
	RSRC_RESP_ERR
};

enum
{
	RSRC_DONE_IP  = (1 << 0),
	RSRC_DONE_OP  = (1 << 1),
	RSRC_DONE_IO  = (RSRC_DONE_IP | RSRC_DONE_OP),
	RSRC_FLUSHING = (1 << 2),
	RSRC_DONE_ERR = (1 << 3)
};

enum
{
	REQ_WAIT_HEAD = (-1),
	REQ_WAIT_BODY = (-2),
	REQ_COMPLETE = (-3)
};

enum
{
	RSP_WAIT_HEAD = (-1),
	RSP_WAIT_BODY = (-2),
	RSP_COMPLETE = (-3),
	RSP_ERROR = (-4)
};

// AH : sending partial .. CGI TIMEOUT .. half-sent
// BUT : bigimage .. takes hella ..
// ATTN : fpm
# ifndef RES_CGI_WAIT_COMPLETE
#  define RES_CGI_WAIT_COMPLETE 0
# endif

class ResourceCgi
{
public:
	ResourceCgi(void) :  
		done(0),
		error(0),
		hed(0),
		conn(NULL)
	{}
	virtual ~ResourceCgi() {};

	int				get_req_body(void);
	
	int				recv_data(char *buf, int siz);
	std::string &	get_resp(void) { return (this->resp); }
	void			set_err(int e);
	int				set_done(int d);
	
	virtual void	push_body(void) = 0;
	virtual int		status(void) = 0;
	virtual void	conn_closed(void) = 0;
	virtual int		rem(EpollClient *epc) = 0;
	
	std::string		body; // Request
	std::string		resp; // CGI output
	int				done;
	int				error;
	
protected:
	int				hed;
	
	virtual int		wait(int opt) = 0;
	void			chk_rsp_len(void);
	
private:
	int				chk_rsp_hed(void);
	
protected:
	Connection		*conn;
};


#endif