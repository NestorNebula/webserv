/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MsgBuf.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/03 16:27:53 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/09 14:12:13 by kdonlon          ###   ########.fr       */
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

#ifndef USE_AP
#define USE_AP 0
#endif

#if USE_AP
#include "AlignedPtr.hpp"
#endif

#if 0
class MsgPtr
{
public:
    unsigned char * buf;
    unsigned int    siz;
    unsigned int    beg;
    unsigned int    end;

    MsgPtr()
    {
    }
};

template <class T>
class TypeBuf
{
private:
#if USE_AP
    AlignedPtr    * ap;
#endif
public:
    T             * buf;
    unsigned int    siz;
    unsigned int    beg;
    unsigned int    end;

    TypeBuf(unsigned int n = 64)
    {
#if USE_AP
        this->ap  = NULL;
#endif
        this->zero();
        this->avail(n);
    }
    ~TypeBuf()
    {
#if USE_AP
        if (this->ap)
            delete (this->ap);
#else
        if (this->buf)
            free(this->buf);
#endif
    }
    void zero()
    {
        this->siz = 0;
        this->beg = 0;
        this->end = 0;
        this->buf = NULL;
    }
    T * head()
    {
        return (T*) (this->buf + this->beg);
    }
    T * tail()
    {
        return (this->buf + this->end);
    }
    unsigned int size()
    {
        return (this->end - this->beg);
    }
    void push (T * b, int s)
    {
        this->avail(s);

        memcpy(this->tail(), b, s * sizeof(T));

        this->end += s;
    }

    unsigned int avail(unsigned int n = 0)
    {
        if ( (n == 0) || (n < this->avail()) )
        {
            // if (n) fprintf(stderr, "AVAIL (%i) : ok\n", n);
            return (this->siz - this->end);
        }

        if ((this->beg + this->avail()) >= n)
        {
        	// we have enough data allocated (?)
        	//
            unsigned int ava = this->normal();
            if (n <= ava)
            {
                // fprintf(stderr, "AVAIL (%i) : normalized\n", n);
                return ava;
            }
            fprintf(stderr, "AVAIL (%i) : normalize FAILED\n", n);
        }

        this->siz = (n + this->end);


#if USE_AP
        fprintf(stderr, "MsgBuf : avail (%i) aligned\n", this->siz);
        AlignedPtr * tmp = new AlignedPtr(this->siz * sizeof(T), 64);
        // CHECK
        if (this->ap)
        {
            memcpy(tmp->ptr, this->head(), this->size() * sizeof(T));
            this->end = this->size();
            this->beg = 0;
            delete this->ap;
        }
        this->ap = tmp;
        this->buf = (T*) ap->ptr;
#else
        // fprintf(stderr, "MsgBuf : avail (%i) malloc\n", this->siz);
        T * tmp = (T*) malloc(this->siz * sizeof(T));
        if (this->buf)
        {
            memcpy(tmp, this->head(), this->size() * sizeof(T));
            // does this fuck with SSL ?
            this->end = this->size();
            this->beg = 0;
            free(this->buf);
        }
        this->buf = tmp;
#endif
        return this->avail();
    }
    int clear(int n = -1)
    {
        if ( (n < 0) || ((this->beg + n) >= this->end))
        {
            // fprintf(stderr, "MsgBuf : CLEAR (%i) to ZERO\n", n);
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
            fprintf(stderr, "MsgBuf normal beg %i pad %i\n", this->beg, pad);
            // shift RIGHT .. ugh
        }
        // actually -- we can always normalize
        // just not always in one copy
        if (this->size() > (this->beg - pad))
        {
            unsigned int full_size = this->size();
            int copy_size = this->beg - pad;
            T * src = this->head();
            T * dst = this->buf + pad;

// fprintf(stderr, "COPY CHNK (%u)\n", full_size);
            do
            {
                memcpy(dst, src, copy_size * sizeof(T));
                src += copy_size;
                dst += copy_size;
                this->beg += copy_size;
                if (this->size() < copy_size)
                    copy_size = this->size();
            } while (this->size());

            this->beg = pad;
            this->end = full_size + pad;
            return this->avail();
        }
        // fprintf(stderr, "NORMAL\n");

// fprintf(stderr, "COPY FULL (%u)\n", this->size());

        memcpy(this->buf + pad, this->head(), this->size() * sizeof(T));

        this->end = this->size() + pad;
        this->beg = pad;

        return this->avail();
    }
    void dump()
    {
        fprintf(stderr, "BUF (%u - %u) (%u / %u)\n", this->beg, this->end, this->size(), this->siz);
    }
};


class StreamBuf
{
public:
    unsigned char * buf;
    unsigned int    siz;
    unsigned int    beg;
    unsigned int    end;
    unsigned int    stp;

    StreamBuf(unsigned int n = (1024 * 1024))
    {
        this->stp = n;

        this->siz = 0;
        this->beg = 0;
        this->siz = 0;
        this->end = 0;

        this->set_siz(n);
    }
    ~StreamBuf()
    {
        if (this->buf)
            free(this->buf);
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
    unsigned int avail()
    {
        return (this->siz - this->end);
    }
    void push(void * v, int s)
    {
        if (s < (int) this->avail())
        {
            memcpy(this->tail(), v, s);
            this->end += s;
            return;
        }
        fprintf(stderr, "StreamBuf : re-alloc\n");
        this->set_siz(this->end + this->stp);

        memcpy(this->tail(), v, s);
        this->end += s;
    }
    // set_stp()
    void set_siz(unsigned int z)
    {
        this->siz = z;
        unsigned char * tmp = (unsigned char*) malloc(this->siz);
        if (this->buf)
        {
            // beg    : always (0)
            // head() : always (this->buf)
            // size() : always (this->end)
            memcpy(tmp, this->head(), this->size());

            this->end = this->size();
            this->beg = 0;

            free(this->buf);
        }
        this->buf = tmp;
    }
};
#endif


class MsgBuf
{
private:
#if USE_AP
    AlignedPtr    * ap;
#endif
public:
    unsigned char * buf;
    unsigned int    siz;
    unsigned int    beg;
    unsigned int    end;

    MsgBuf(int n = 64)
    {
#if USE_AP
        this->ap  = NULL;
#endif
        this->zero();
        this->avail(n);
    }
    ~MsgBuf()
    {
#if USE_AP
        if (this->ap)
            delete (this->ap);
#else
        if (this->buf)
            free(this->buf);
#endif
    }
    // memset
    void zero()
    {
        this->siz = 0;
        this->beg = 0;
        this->end = 0;
        this->buf = NULL;
    }
    static unsigned short make_str(unsigned int id, char * str, char * tgt)
    {
        unsigned short len = (unsigned short) strlen(str) + 1; // null-t

        *(unsigned int*) tgt = id;
        tgt += sizeof(unsigned int);

        *(unsigned short*) tgt = htons(len);
        tgt += sizeof(unsigned short);
        strcpy(tgt, str);

        return len + 6; // len + uint + short ? (NULL)
    }
#if 1 // MSG_BUF_STR
        // pseduo-pascal-string with null-terminator
    // ushort strlen
    // char   str[]
    char * str()
    {
        char * src = this->text();
        unsigned short len = ntohs(*(unsigned short*) src);

        src += sizeof(unsigned short);

        if (src[len-1] != '\0')
        {
            fprintf(stderr, "MsgBuf : bad str (%i)\n", len);
            for (int i=0; i < len; i++)
                fprintf(stderr, "%c", src[i]);
            fprintf(stderr, "\n");
            return NULL;
        }
        this->clear(sizeof(unsigned short) + len);
        return src;

        // char * dst = (char*) malloc(len+1);
        // memcpy(dst, src, len);
        // dst[len] = '\0';
        // return dst;
    }
#endif
    // NOT HERE
#if 0
    char * http_chnk()
    {
        // Mp3File.cpp !!!

        const unsigned short crlf = (10 << 8) | 13; // endian flip

        char * src = this->text();
        int    len = 0;
        for (int i=0; i < this->size(); i++)
        {
            if (!memcmp(src+i, &crlf, 2))
            {
                src[i] = '\0';
                len = (int) strtol((const char*) src, NULL, 16);
                // len = chunk size
                src = src + i + 2;
                break;
            }
        }
        if (len == 0)
            return NULL;

        char * dst = (char*) malloc(len+1);
        memcpy(dst, src, len);
        dst[len] = '\0';
        return dst;
    }
#endif
    // unsigned int hed()
    // FourCC
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
    void push(void * v, int s)
    {
        this->avail(s);

        memcpy(this->tail(), v, s);

        this->end += s;
    }
    void push_pstr(char * str)
    {
        unsigned char len = (unsigned char) strlen(str);
        this->push(&len, sizeof(unsigned char));
        this->push(str, len);
    }
    // something different for saving video stream
    // want : fewer re-allocs
    // chunk_size : (10MB)
    // if (s < (this->siz - this->end))
        // we are find
    // else
        // this->avail(10,000,000) // chunk_size

    void zero(int cnt)
    {
        this->avail(cnt);

        memset(this->tail(), 0, cnt);
        this->end += cnt;
    }
    void fcgi(const char * key, char * val)
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
            // fprintf(stderr, "fcgi : key (%i)\n", k);
            k |= 1 << 31;
            k  = htonl(k);
            memcpy(tgt, &k, sizeof(int));
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
            // fprintf(stderr, "fcgi : val (%i)\n", v);
            v |= 1 << 31;
            v  = htonl(v);
            memcpy(tgt, &v, sizeof(int));
            // fprintf(stderr, "fcgi : [ %i %i %i %i ]\n", tgt[0], tgt[1], tgt[2], tgt[3]);
            tgt += sizeof(int);
            z   += sizeof(int);
        }
        else
        {
            *tgt++ = v;
            z     += sizeof(char);
        }
        sprintf(tgt, "%s%s", key, val);

        this->end += z;
    }
    void push(int s) // skip
    {
        this->avail(s);
        this->end += s;
    }
    void force(void * v, int s)
    {
        // push .. without normalize
        // if (s == 0)
            // s = strlen((char*)v);
        this->extend(s);

        memcpy(this->tail(), v, s);

        this->end += s;
    }
    unsigned int extend(unsigned int n = 0)
    {
        if ( (n == 0) || (n < this->extend()) )
            return (this->siz - this->end);

        this->siz = this->end + n;
        // fprintf(stderr, "msg buf siz (%i)\n", this->siz);
#if USE_AP
        // fprintf(stderr, "extend (%i + %i = %i)\n", end, n, siz);
        AlignedPtr * tmp = new AlignedPtr(this->siz, 64);
        if (tmp->siz == 0)
            fprintf(stderr, "FLORIDA!\n");
        if (this->ap)
        {
            // fprintf(stderr, "copy ...\n");
            memcpy(tmp->ptr, this->buf, this->end);
            // fprintf(stderr, "copied\n");
            delete this->ap;
        }
        else
        {
            fprintf(stderr, "MsgBug : ATTN ! new AlignedPtr failed\n");
        }
        this->ap  = tmp;
        this->siz = ap->siz;
        this->buf = (unsigned char*) ap->ptr;
#else
        // ATTN : invalid chunk size ..
        // if we're doing this a LOT (CurlConn)
        fprintf(stderr, "extend (%i + %i = %i)\n", end, n, siz);
        unsigned char * tmp = (unsigned char*) malloc(this->siz);
        if (tmp == NULL)
            fprintf(stderr, "FLORIDA!\n");
        if (this->buf)
        {
            memcpy(tmp, this->buf, this->end);
            free(this->buf);
        }
        this->buf = tmp;
#endif
        return this->extend();
    }
    unsigned int avail(unsigned int n = 0)
    {
        if ( (n == 0) || (n < this->avail()) )
            return (this->siz - this->end);

        if ((this->beg + this->avail()) >= n)
        {
            unsigned int ava = this->normal(); // copy to head (?)
            if (n <= ava)
            {
                // fprintf(stderr, "AVAIL (%i) : normalized\n", n);
                return ava;
            }
            fprintf(stderr, "AVAIL (%i) : normalize FAILED\n", n);
        }

        this->siz = this->end + n;

#if USE_AP
        // fprintf(stderr, "MsgBuf : avail (%i) aligned\n", this->siz);
        AlignedPtr * tmp = new AlignedPtr(this->siz, 64);
        if (tmp->siz == 0)
            fprintf(stderr, "FLORIDA!\n");
        if (this->ap) // ->siz)
        {
            memcpy(tmp->ptr, this->head(), this->size());
            this->end = this->size();
            this->beg = 0;
            delete this->ap;
        }
        this->ap  = tmp;
        this->siz = ap->siz;
        this->buf = (unsigned char*) ap->ptr;
#else
        // fprintf(stderr, "MsgBuf : avail (%i) malloc\n", this->siz);
        unsigned char * tmp = (unsigned char*) malloc(this->siz);
        if (tmp == NULL)
            fprintf(stderr, "FLORIDA!\n");
        if (this->buf)
        {
            memcpy(tmp, this->head(), this->size());
            this->end = this->size();
            this->beg = 0;
            free(this->buf);
        }
        this->buf = tmp;
#endif
        return this->avail();
    }
    int clear(int n = -1)
    {
        if ( (n < 0) || ((this->beg + n) >= this->end))
        {
            // fprintf(stderr, "MsgBuf : CLEAR (%i) to ZERO\n", n);
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
            fprintf(stderr, "MsgBuf normal beg %i pad %i\n", this->beg, pad);
            // shift RIGHT .. ugh
        }
        // actually -- we can always normalize
        // just not always in one copy
        if (this->size() > (this->beg - pad))
        {
            // fprintf(stderr, "MsgBuf - normal FAILED\n");
            unsigned int full_size = this->size();
            int copy_size = this->beg - pad;
            unsigned char * src = this->head();
            unsigned char * dst = this->buf + pad;

            do
            {
                memcpy(dst, src, copy_size);
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
        // fprintf(stderr, "NORMAL\n");
        memcpy(this->buf + pad, this->head(), this->size());
        this->end = this->size() + pad;
        this->beg = pad;

        return this->avail();
    }
    void dump()
    {
        fprintf(stderr, "  %u - %u; %u / %u\n", this->beg, this->end, this->size(), this->siz);
    }
    void hexdump(int cnt = 0)
    {
        if (!cnt)
            cnt = this->size();

        unsigned char * chk = this->head();

        fprintf(stderr, "%02x ", chk[0]);
        for (int i=1 ; i < cnt; i++)
        {
            if (!(i % 16))
                fprintf(stderr, "\n");
            else if (!(i % 8))
                fprintf(stderr, " ");
            fprintf(stderr, "%02x ", chk[i]);
        }
        fprintf(stderr, "\n\n");
    }
    void echo()
    {
        char * c = (char*) this->buf;
        char * h = (char*) this->head();
        char * t = (char*) this->tail();
        char * x = (char*) this->buf + this->siz;

        while (c < x)
        {
            if (c < h)
                fprintf(stderr, "<");
            else if (c < t)
                fprintf(stderr, "%c", *c);
            else
                fprintf(stderr, ">");
            c++;
        }
        fprintf(stderr, "\n");
    }
};

// multi-declared in
    // lib_luamgr, vcMsg.hpp/cpp
// inline void buf_push(char ** buf, void * p, long int siz)
// {
//     memcpy((void*) *buf, p, siz);
//     *buf += siz;
// }

// used in
    // snapshot functions
    // keymgr
    // param object
    // param randomizer
    // sequence
#endif