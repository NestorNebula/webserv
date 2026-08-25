/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SizeDefs.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 17:56:17 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/25 08:50:19 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIZE_DEFS_HPP
# define SIZE_DEFS_HPP

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



# ifndef CONN_TIMEOUT
#  define CONN_TIMEOUT 60
# endif

# ifndef CGI_TIMEOUT
#  define CGI_TIMEOUT 60
# endif


# ifndef RES_CGI_WAIT_COMPLETE
    // .. except if content-length ... 
#  define RES_CGI_WAIT_COMPLETE 1
# endif


// git fetch --all                                   # Get the latest code
// git checkout -b my-single-change upstream/master  # Create new branch based on upstream/master
// git cherry-pick b50b2e7                           # Cherry pick the commit you want
// git push -u origin my-single-change               # Push your changes to the remote branch
#endif


