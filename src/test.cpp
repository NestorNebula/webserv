/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/26 10:09:16 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Epoll.hpp"
#include "Server.hpp"
#include "Connection.hpp"

#if 0
https://man7.org/linux/man-pages/man7/epoll.7.html
https://man7.org/linux/man-pages/man2/epoll_ctl.2.html

#endif


// static volatile sig_atomic_t stop = 0;

// c
// static void handle_sigint(int signo)
// {
//     (void)signo;
//     stop = 1;
// }

    // struct sigaction sa = {
    //     .sa_handler = handle_sigint,
    // };
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0; /* no SA_RESTART */

    // if (sigaction(SIGINT, &sa, NULL) < 0) {
    //     perror("sigaction");
    //     return 1;
    // }

// capture SIGINT -- cleanup nicely

        //     struct timespec timeout = {
        //     .tv_sec = 60,
        //     .tv_nsec = 0,
        // };
    // while (!stop) {

int main (void)
{
    int err;
    
    Epoll ep;

    Server s0(ep, 8080);
    Server s1(ep, 8081);
    
    err = s0.init();
    if (err < 0)
    {
        return (0);
    }
    err = s1.init();
    if (err < 0)
    {
        return (0);
    }
    // addClient()
    ep.add(s0.get_fd(), EPOLLIN, &s0);
    ep.add(s1.get_fd(), EPOLLIN, &s1);
    
    err = ep.loop();

    return (err);
}
