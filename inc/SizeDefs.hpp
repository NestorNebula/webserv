/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SizeDefs.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 17:56:17 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 11:38:59 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIZE_DEFS_HPP
# define SIZE_DEFS_HPP


# define DEF_BUF_SIZ 4096

// MUST BE : > (8) for FCGI	
// #nh says we can keep these
# ifndef EPC_BUF_SIZ
#  define EPC_BUF_SIZ (DEF_BUF_SIZ)
# endif

# ifndef EPC_OUT_SIZ
#  define EPC_OUT_SIZ (DEF_BUF_SIZ)
# endif

# ifndef STREAM_READ_SIZ
#  define STREAM_READ_SIZ (DEF_BUF_SIZ)
# endif

# ifndef REQ_READ_SIZ
#  define REQ_READ_SIZ (DEF_BUF_SIZ)
# endif

# ifndef RSP_READ_SIZ
#  define RSP_READ_SIZ (DEF_BUF_SIZ)
# endif



# ifndef CONN_TIMEOUT
#  define CONN_TIMEOUT 60
# endif

# ifndef CGI_TIMEOUT
#  define CGI_TIMEOUT 60
# endif


# ifndef RES_CGI_WAIT_COMPLETE
#  define RES_CGI_WAIT_COMPLETE 0
# endif





#endif
