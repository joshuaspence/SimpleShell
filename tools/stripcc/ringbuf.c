/**
 *  File: ringbuf.c
 *                                                                          
 *  Copyright (C) 2008 Du XiaoGang <dugang@188.com>                         
 *                                                                          
 *  This file is part of stripcc.                                          
 *                                                                          
 *  stripcc is free software: you can redistribute it and/or modify        
 *  it under the terms of the GNU General Public License as                 
 *  published by the Free Software Foundation, either version 3 of the      
 *  License, or (at your option) any later version.                         
 *                                                                          
 *  stripcc is distributed in the hope that it will be useful,             
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           
 *  GNU General Public License for more details.                            
 *                                                                          
 *  You should have received a copy of the GNU General Public License       
 *  along with stripcc.  If not, see <http://www.gnu.org/licenses/>.       
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ringbuf.h"

struct ringbuf_t *
ringbuf_new(int capacity)
{
    struct ringbuf_t *ret;

    ret = malloc(sizeof(struct ringbuf_t));
    if (ret == NULL)
        return NULL;
    ret->buf = malloc(capacity);
    if (ret->buf == NULL) {
        free(ret);
        return NULL;
    }
    ret->cap = capacity;
    ret->idx = 0;
    ret->writed = 0;

    return ret;
}

void
ringbuf_clear(struct ringbuf_t *rbuf)
{
    rbuf->idx = 0;
    rbuf->writed = 0;
}

void
ringbuf_write(struct ringbuf_t *rbuf, const void *data, int len)
{
    int rest;

    if (len == -1) {
        /* data is a string */
        len = strlen((char *)data);
    }
    if (len > rbuf->cap) {
        /* only the last cap's length data will be writen to ringbuf */
        data = data + len - rbuf->cap;
        len = rbuf->cap;
    }
    rest = rbuf->cap - rbuf->idx;
    if (len >= rest) {
        /* roll back */
        memcpy(rbuf->buf + rbuf->idx, data, rest);
        rbuf->idx = 0;
        rbuf->writed += rest;
        /* update data/len */
        data += rest;
        len -= rest;
    }
    if (len > 0) {
        memcpy(rbuf->buf + rbuf->idx, data, len);
        rbuf->idx += len;
        rbuf->writed += len;
    }
}

int
ringbuf_read_last(struct ringbuf_t *rbuf, void *buf, int len)
{
    int ret_len, part1_len;

    if (rbuf->writed > rbuf->cap)
        ret_len = rbuf->cap;
    else
        ret_len = rbuf->writed;
    if (ret_len > len)
        ret_len = len;
    /* copy */
    part1_len = ret_len - rbuf->idx;
    if (part1_len > 0) {
        memcpy(buf, rbuf->buf + rbuf->cap - part1_len, part1_len);
        buf += part1_len;
        memcpy(buf, rbuf->buf, rbuf->idx);
    } else {
        memcpy(buf, rbuf->buf + rbuf->idx - ret_len, ret_len);
    }
    return ret_len;
}

void
ringbuf_free(struct ringbuf_t *rbuf)
{
    free(rbuf->buf);
    free(rbuf);
}
