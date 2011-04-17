/**
 *  File: list.h
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

#define _LIST_H_

struct list_t {
    struct list_t *next;
    void *data;
};

typedef void (*item_free_func)(struct list_t *);
typedef int (*data_cmp_func)(const void *list_data, const void *data);

void list_free(struct list_t *list, item_free_func nfree);
struct list_t *sorted_list_insert_item(struct list_t *list, struct list_t *item, 
                                       data_cmp_func dcmp);
struct list_t *sorted_list_remove_and_free_item_by_data(struct list_t *list, 
                                                        const void *data, 
                                                        data_cmp_func dcmp, 
                                                        item_free_func nfree);
struct list_t *list_prepend_item_by_data(struct list_t *list, const void *data);
struct list_t *list_search_item_by_data(struct list_t *list, const void *data);

