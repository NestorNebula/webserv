/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsTime.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 10:45:16 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/04 09:43:03 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WS_TIME_HPP
# define WS_TIME_HPP

# include <ctime>

# ifndef EXTRA_TIME
#  define EXTRA_TIME 0
# endif

# if EXTRA_TIME
typedef struct timespec wstime_t;
# else
typedef time_t wstime_t;
#endif

class WsTime
{
public:
    wstime_t  t;
    
    WsTime(void)
    {
# if EXTRA_TIME
        this->t.tv_sec = 0;
        this->t.tv_nsec = 0;
#else
        this->t = 0;
#endif
    }
    WsTime(wstime_t v)
    {
        this->t = v;
    }
	WsTime (const WsTime & that)
    {
        this->t = that.t;
    }
	WsTime & operator = (const WsTime & that)
    {
        if (this == &that)
            return (*this);
        this->t = that.t;
        return (*this);
    }

    void    set_now(void)
    {
# if EXTRA_TIME
        // gettimeofday(&t);
        clock_gettime(CLOCK_MONOTONIC, &t);
# else 
        std::time(&t);
#endif
    }
    bool    not_set(void)
    {
        return (this->t == 0);
    }
    bool    after(WsTime wst)
    {
        return (wst.t < this->t);
    }
    bool    before(WsTime wst)
    {
        return (wst.t > this->t);
    }

    bool operator == (WsTime const& that)
    {
        return (this->t == that.t);
    }

    bool operator == (wstime_t _t)
    {
        return (this->t == _t);
    }

    WsTime operator + (const double secs) const
    {
        return WsTime(this->t + secs);
    }
    WsTime operator - (const double secs) const
    {
        return WsTime(this->t - secs);
    }
    bool operator < (WsTime const& that)
    {
        return (this->t < that.t);
    }
    bool operator > (WsTime const& that)
    {
        return (this->t > that.t);
    }
};

#endif