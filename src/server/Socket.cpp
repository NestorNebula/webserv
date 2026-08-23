/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:16:45 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 11:16:31 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Socket.hpp"

int sock_non_block(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return (-1);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK); //  | O_CLOEXEC);
}

// SOCK_CLOEXEC
// Set the close-on-exec (FD_CLOEXEC) flag on the new file
// descriptor.  See the description of the O_CLOEXEC flag in
// open(2) for reasons why this may be useful.

std::string addr_2_str(struct sockaddr_in *addr)
{
    std::stringstream ss;

    // ntohl (addr->sin_addr.s_addr);
	unsigned char *a = reinterpret_cast<unsigned char*>(&addr->sin_addr.s_addr);
    ss << (int) a[0] << "." << (int) a[1] << "."<< (int) a[2] << "."<< (int) a[3] << ":" << ntohs(addr->sin_port);
    
    return (ss.str());
}
