/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   test.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/19 11:24:22 by kdonlon           #+#    #+#             */
/*   Updated: 2026/06/21 14:39:21 by kdonlon          ###   ########.fr       */
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
    ep.add(s0.get_fd(), EPOLLIN, &s0);
    ep.add(s1.get_fd(), EPOLLIN, &s1);
    
    // ep.loop()
    while (1)
    {
        int e = ep.exec(); // ep.ecnt
        if (e == 0)
            ; // timeout
        else if (e < 0)
            break ; // error
        while (e--)
        {
            struct epoll_event	*evt = ep.get_evt(e);
            evt_typ(evt);
            EpollClient * epc = reinterpret_cast<EpollClient*>(evt->data.ptr);
            if (evt->events & EPOLLRDHUP)
            {
                // ep.del() // need fd .. 
                // remove "Connection" from "Server"
                // ep.hup .. 
                // then .. delete it -- is that a good idea (?)
                // or .. delete Connection -- 
                    // removes itself from the Server in destructor ..
                    // assumes : Server still exists (?)
                // POLLIN -- also set .. but ... 
                // read returns (0) .. so .. 
                // ready-to-read .. but returns (0) .. means
                // EOF reached on INPUT 
#if 0                    
                int rfd = epc->get_fd();
                epc->pollout();
                // NOT QUITE -- we can still write to it ...
                ep.del(rfd);
                close(rfd);
                // EOF 
                // CLOSE CONNECTION HERE

                continue;
#endif                
            }


            if (evt->events & EPOLLIN)
            {
                if (epc)
                {
                    err = epc->pollin();
                }
            }
            if (evt->events & EPOLLOUT)
            {
                if (epc)
                {
                    err = epc->pollout();
                }
            }
        }
    }
    return (0);
}
