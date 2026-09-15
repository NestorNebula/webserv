/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:16:23 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/15 09:54:43 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "fcntl.h"
# include <unistd.h>
# include <arpa/inet.h>
# include <string>
# include <sstream>

int         sock_non_block(int fd);
std::string addr_2_str(struct sockaddr_in * addr);

int         fd_close(int *fd);

int         sock_file(const char *path);

#endif