/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SizeDefs.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 17:56:17 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/02 12:01:28 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIZE_DEFS_HPP
# define SIZE_DEFS_HPP

# ifndef CONN_TIMEOUT
#  define CONN_TIMEOUT 60
# endif

# ifndef CGI_TIMEOUT
#  define CGI_TIMEOUT 60
# endif

# ifndef CGI_RETRY_INTERVAL
#  define CGI_RETRY_INTERVAL 1
# endif

// We can get into a loop here ... 
// what .. conns fill ALL (fd)
// so impossible to create CGI to flush 
# ifndef CGI_RETRY_COUNT
#  define CGI_RETRY_COUNT 10
# endif


# define DEF_BUF_SIZ 8192

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


# ifndef SYSCALL_ERR
#  define SYSCALL_ERR (-2)
# endif

#endif


