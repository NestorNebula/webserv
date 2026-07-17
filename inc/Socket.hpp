/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:16:23 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/14 20:10:34 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "fcntl.h"
# include <arpa/inet.h>
# include <string>
# include <sstream>

inline int sock_non_block(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return (-1);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

std::string addr_2_str(struct sockaddr_in * addr);

#endif