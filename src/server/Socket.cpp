/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:16:45 by kdonlon           #+#    #+#             */
/*   Updated: 2026/07/07 17:11:44 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Socket.hpp"


std::string addr_2_str(struct sockaddr_in *addr)
{
    std::stringstream ss;

    // ntohl .. 
	unsigned char *a = reinterpret_cast<unsigned char*>(&addr->sin_addr.s_addr);
    ss << (int) a[0] << "." << (int) a[1] << "."<< (int) a[2] << "."<< (int) a[3] << ":" << ntohs(addr->sin_port);
    return (ss.str());
}