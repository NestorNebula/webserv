/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WsTime.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/03 10:45:16 by kdonlon           #+#    #+#             */
/*   Updated: 2026/09/03 11:55:51 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WS_TIME_HPP
# define WS_TIME_HPP

# include <ctime>

class WsTime
{
public:
    time_t  t;
    WsTime(void)
    {
        this->t = 0;
    }
    WsTime(time_t v)
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
        std::time(&t);
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

    bool operator == (time_t _t)
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
    
// over-ride +/-

private:
};

#endif