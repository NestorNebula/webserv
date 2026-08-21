/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MsgBuf.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:53 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/21 03:55:54 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MSG_BUF_HPP
#define MSG_BUF_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// DOC_OPT/src


#ifdef __WIN32__
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "libft.hpp"
#include "WsLog.hpp"


class MsgBuf
{
public:
    unsigned char * buf;
    unsigned int    siz;
    unsigned int    beg;
    unsigned int    end;

    MsgBuf(int n = 64)
    {
        this->zero();
        this->avail(n);
    }
    ~MsgBuf()
    {
        if (this->buf)
            free(this->buf);
    }
    void zero()
    {
        this->siz = 0;
        this->beg = 0;
        this->end = 0;
        this->buf = NULL;
    }

    char * text()
    {
        return (char*) (this->buf + this->beg);
    }
    unsigned char * head()
    {
        return (this->buf + this->beg);
    }
    unsigned char * tail()
    {
        return (this->buf + this->end);
    }
    unsigned int    size()
    {
        return (this->end - this->beg);
    }
    void skip(int s)
    {
        this->avail(s);
        this->end += s;
    }
    void push(const void * v, int s)
    {
        this->avail(s);

        ft_memcpy(this->tail(), v, s);

        this->end += s;
    }
    void zero(int cnt)
    {
        this->avail(cnt);

        ft_memset(this->tail(), 0, cnt);
        this->end += cnt;
    }
    void fcgi(const char * key, const char * val)
    {
        int k = strlen(key);
        int v = strlen(val);
        int z = k + v;

        this->avail(z + sizeof(int) + sizeof(int));
/*
php-fpm fastcgi.c
        val_len = *p++;
        if (val_len >= 128) {
            val_len = ((val_len & 0x7f) << 24);
            val_len |= (*p++ << 16);
            val_len |= (*p++ << 8);
            val_len |= *p++;
        }
*/
        char  * tgt = (char*) this->tail();

        if (k >= 128)
        {
            WSCOL(WSL_RED);
            WSLOG(LVL_DBG, TGT_FCGI, "keylen ", k);
            k |= 1 << 31;
            k  = htonl(k);
            ft_memcpy(tgt, &k, sizeof(int));
            tgt += sizeof(int);
            z   += sizeof(int);
        }
        else
        {
            *tgt++ = k;
            z     += sizeof(char);
        }
        if (v >= 128)
        {

            WSCOL(WSL_RED);
            WSLOG(LVL_DBG, TGT_FCGI, "vallen ", k);
            v |= 1 << 31;
            v  = htonl(v);
            ft_memcpy(tgt, &v, sizeof(int));
            tgt += sizeof(int);
            z   += sizeof(int);
        }
        else
        {
            *tgt++ = v;
            z     += sizeof(char);
        }
        // sprintf(tgt, "%s%s", key, val);
        const char *src = key;
        while (*src)
            *tgt++ = *src++;
        src = val;
        while (*src)
            *tgt++ = *src++;

        this->end += z;
    }
    void push(int s) // skip
    {
        this->avail(s);
        this->end += s;
    }
    unsigned int avail(unsigned int n = 0)
    {
        if ( (n == 0) || (n < this->avail()) )
            return (this->siz - this->end);

        if ((this->beg + this->avail()) >= n)
        {
            unsigned int ava = this->normal(); // copy to head (?)
            if (n <= ava)
                return ava;
        }

        this->siz = this->end + n;

        unsigned char * tmp = (unsigned char*) malloc(this->siz);
        if (this->buf)
        {
            ft_memcpy(tmp, this->head(), this->size());
            this->end = this->size();
            this->beg = 0;
            free(this->buf);
        }
        this->buf = tmp;
        return this->avail();
    }
    int clear(int n = -1)
    {
        if ( (n < 0) || ((this->beg + n) >= this->end))
        {
            this->beg = 0;
            this->end = 0;
            return 0;
        }
        this->beg += n;

        // old PingConn always tried to Normal here

        return 1;
    }
    unsigned int normal(unsigned int pad = 0)
    {
        if (this->beg == pad)
            return this->avail();

        if (this->beg < pad)
        {
            // shift RIGHT .. ugh
        }
        // actually -- we can always normalize
        // just not always in one copy
        if (this->size() > (this->beg - pad))
        {
            unsigned int full_size = this->size();
            int copy_size = this->beg - pad;
            unsigned char * src = this->head();
            unsigned char * dst = this->buf + pad;

            do
            {
                ft_memcpy(dst, src, copy_size);
                src += copy_size;
                dst += copy_size;
                this->beg += copy_size;
                if ((int) this->size() < copy_size)
                    copy_size = this->size();
            } while (this->size());

            this->beg = pad;
            this->end = full_size + pad;
            return this->avail();
        }
        ft_memcpy(this->buf + pad, this->head(), this->size());
        this->end = this->size() + pad;
        this->beg = pad;

        return this->avail();
    }
};

#endif