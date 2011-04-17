/**
 *  File: list.c
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

#include "list.h"

void
list_free(struct list_t *list, item_free_func nfree)
{
    struct list_t *tmp;

    tmp = list;
    while (tmp != NULL) {
        list = list->next;
        /* free */
        nfree(tmp);
        /* next */
        tmp = list;
    }
}

struct list_t *
sorted_list_insert_item(struct list_t *list, struct list_t *item, data_cmp_func dcmp)
{
    struct list_t *tmp, *prev;
    int ret;

    if (list == NULL) {
        item->next = NULL;
        return item;
    }
    /* small --> big */
    tmp = list;
    prev = NULL;
    while (tmp != NULL) {
        ret = dcmp(tmp->data, item->data);
        if (ret > 0) {
            /* insert item before tmp */
            if (prev == NULL) {
                /* item should be the first */
                item->next = list;
                return item;
            } else {
                item->next = tmp;
                prev->next = item;
                return list;
            }
        } else if (ret == 0) {
            /* item is already exist */
            return NULL;
        }
        /* next */
        prev = tmp;
        tmp = tmp->next;
    }
    /* append item to list */
    item->next = NULL;
    prev->next = item;
    return list;
}

struct list_t *
sorted_list_remove_and_free_item_by_data(struct list_t *list, const void *data, 
                                         data_cmp_func dcmp, item_free_func nfree)
{
    struct list_t *tmp, *prev;
    int ret;

    /* small --> big */
    tmp = list;
    prev = NULL;
    while (tmp != NULL) {
        ret = dcmp(tmp->data, data);
        if (ret == 0) {
            /* remove */
            if (prev == NULL) {
                list = tmp->next;
                nfree(tmp);
                return list;
            } else {
                prev->next = tmp->next;
                nfree(tmp);
                return list;
            }
        }
        /* next */
        prev = tmp;
        tmp = tmp->next;
    }
    /* not found */
    return list;
}

struct list_t *
list_prepend_item_by_data(struct list_t *list, const void *data)
{
    struct list_t *rets;

    rets = malloc(sizeof(struct list_t));
    if (rets == NULL)
        return NULL;
    rets->next = list;
    rets->data = (void *)data;
    return rets;
}

struct list_t *
list_search_item_by_data(struct list_t *list, const void *data)
{
    while (list != NULL) {
        if (list->data == data)
            return list;
        /* next */
        list = list->next;
    }
    return NULL;
}
