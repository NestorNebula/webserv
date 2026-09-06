/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsTime.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 10:45:16 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/06 11:38:01 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WS_TIME_HPP
# define WS_TIME_HPP

# include <ctime>
# include <cmath>

# ifndef EXTRA_TIME
#  define EXTRA_TIME 1
# endif

# define NSEC 1000000000L

class TimeSpec
{
private:
    struct timespec t;
public:
    TimeSpec(void)
    {
        this->t.tv_sec  = 0;
        this->t.tv_nsec = 0;
    }
    TimeSpec(struct timespec v)
    {
        this->t = v;
    }
    TimeSpec(time_t v)
    {
        this->t.tv_sec  = v;
        this->t.tv_nsec = 0;
    }
	TimeSpec (const TimeSpec & that)
    {
        this->t = that.t;
    }
	TimeSpec & operator = (const TimeSpec & that)
    {
        if (this == &that)
            return (*this);
        this->t.tv_sec  = that.t.tv_sec;
        this->t.tv_nsec = that.t.tv_nsec;
        return (*this);
    }

    void    normalize(void)
    {
        while (this->t.tv_nsec < 0)
        {
            this->t.tv_sec--;
            this->t.tv_nsec += NSEC;
            return;
        }
        while (this->t.tv_nsec > NSEC)
        {
            this->t.tv_sec++;
            this->t.tv_nsec -= NSEC;
        }
    }
    bool operator == (TimeSpec const& that)
    {
        return
            (this->t.tv_sec  == that.t.tv_sec)
            &&
            (this->t.tv_nsec == that.t.tv_nsec);
    }
    // bool operator == (wstime_t _t)
    // {
    //     return (this->t == _t);
    // }
    TimeSpec operator + (const double secs) const
    {
        TimeSpec tmp = *this;

        double ipart;
        double fpart = std::modf(secs, &ipart);

        tmp.t.tv_sec  += ipart;
        tmp.t.tv_nsec += (fpart * NSEC);
        tmp.normalize();
        return (tmp);
    }
    TimeSpec operator - (const double secs) const
    {
        TimeSpec tmp = *this;

        double ipart;
        double fpart = std::modf(secs, &ipart);

        tmp.t.tv_sec  -= ipart;
        tmp.t.tv_nsec -= (fpart * NSEC);
        tmp.normalize();
        return (tmp);
    }
    bool operator < (TimeSpec const& that)
    {
        if (this->t.tv_sec < that.t.tv_sec)
            return (true);
        if (this->t.tv_sec > that.t.tv_sec)
            return (false);
        return (this->t.tv_nsec < that.t.tv_nsec);
    }
    bool operator > (TimeSpec const& that)
    {
        if (this->t.tv_sec > that.t.tv_sec)
            return (true);
        if (this->t.tv_sec < that.t.tv_sec)
            return (false);
        return (this->t.tv_nsec > that.t.tv_nsec);
    }
};

# if EXTRA_TIME
typedef TimeSpec wstime_t;
# else
typedef time_t wstime_t;
# endif

class WsTime
{
private:
# if EXTRA_TIME
    TimeSpec    t;
# else
    wstime_t    t;
# endif
public:

    WsTime(void)
    {
        this->t = 0;
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
        clock_gettime(CLOCK_MONOTONIC, (struct timespec*) &t);
# else
        std::time(&t);
# endif
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
    // bool operator == (wstime_t _t)
    // {
    //     return (this->t == _t);
    // }
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