/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:16:45 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/15 09:55:41 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Socket.hpp"

#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

int sock_non_block(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return (-1);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC);
}

std::string addr_2_str(struct sockaddr_in *addr)
{
    std::stringstream ss;

    // ntohl (addr->sin_addr.s_addr);
	unsigned char *a = reinterpret_cast<unsigned char*>(&addr->sin_addr.s_addr);
    ss << (int) a[0] << "." << (int) a[1] << "."<< (int) a[2] << "."<< (int) a[3] << ":" << ntohs(addr->sin_port);

    return (ss.str());
}

int fd_close(int *fd)
{
	if (*fd == -1)
		return (1);

	int err = close(*fd);
	*fd = -1;
    return (err);
}

int sock_file(const char *path)
{
    if (access(path, R_OK | W_OK))
        return (0);

    struct stat st;
    if (stat(path, &st))
        return (0);
    return ((st.st_mode & S_IFMT) == S_IFSOCK);
}
