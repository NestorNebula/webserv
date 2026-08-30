/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResourceCgi.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:30:46 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/30 16:44:35 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESOURCE_CGI_HPP
# define RESOURCE_CGI_HPP

# include "SizeDefs.hpp"
# include "helpers.hpp"
# include "Connection.hpp"
# include "TemporaryFileStream.hpp"

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
	RSP_KPALIVE = (-4),
	RSP_ERROR = (-5)
};

class ResourceCgi
{
public:
	ResourceCgi(void) :  
		done(0),
		error(0),
		ka(false),
		hed(0),
// RES_CGI_WAIT_COMPLETE
		wait_comp(false),
		tfs(NULL),
		conn(NULL)
	{}
	virtual ~ResourceCgi()
	{
		if (this->tfs)
		{
			WSCOL(WSL_YELLOW);
			WSLOG(LVL_DBG, TGT_RSRC, "dele: temp file");
			delete (this->tfs);
		}
	};

	int				get_req_body(void);
	
	int				recv_data(char *buf, int siz);
	bool			resp_data(void);
	
	std::string &	get_resp(void);
	int				set_err(int e);
	int				set_done(int d);
	
	virtual void	push_body(void) = 0;
	virtual int		status(void) = 0;
	virtual void	conn_closed(void) = 0;
	virtual int		rem(EpollClient *epc) = 0;
	
	std::string		body; // HTTP Request
	std::string		resp; // CGI  Output
	int				done;
	int				error;
	bool			ka;
	
protected:
	int				hed;
// RES_CGI_WAIT_COMPLETE
	bool			wait_comp;
	virtual int		wait(int opt) = 0;
	void			chk_rsp_len(void);
	
private:
	TemporaryFileStream * tfs;
	int				chk_rsp_hed(void);
	
protected:
	Connection		*conn;
};


#endif