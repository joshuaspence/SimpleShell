/**
 *  File: ringbuf.h
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

#define _RINGBUF_H_

struct ringbuf_t {
    char *buf;
    int cap;
    int idx;
    int writed;
};

struct ringbuf_t *ringbuf_new(int capacity);
void ringbuf_clear(struct ringbuf_t *rbuf);
void ringbuf_write(struct ringbuf_t *rbuf, const void *data, int len);
int ringbuf_read_last(struct ringbuf_t *rbuf, void *buf, int len);
void ringbuf_free(struct ringbuf_t *rbuf);

