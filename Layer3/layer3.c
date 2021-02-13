/*
 * =====================================================================================
 *
 *       Filename:  layer3.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-07 12:46:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include "graph.h"
#include "layer3.h"
#include <sys/socket.h>
#include <memory.h>
#include "tcpconst.h"
#include <stdlib.h>
#include "comm.h"
#include <arpa/inet.h>




//TOTO : route lookup function , LPM lookup function



void init_rt_table(rt_table_t **rt_table){
    //calloc mem
    *rt_table = calloc(1,sizeof(rt_table_t));
    //init glthread
    init_glthread((*rt_table)->route_list);
}







l3_route_t * rt_table_lookup(rt_Table_t *rt_table, char *ip_addr, char mask){
    glthread_t *curr;
    l3_route_t *l3_route;


    //loop through routing table
    ITERATE_GLTHREAD_BEGIN(&rt_Table->route_list, curr){
        l3_route = rt_glue_to_l3_route(curr);
        if(strncmp(l3_route->dest,ip_addr,16) == 0 && l3_route->mask == mask){
            return l3_route
        }
    } ITERATE_GLTHREAD_END(&rt_table->route_list,curr);
}









void delete_rt_table_entry(rt_table_t *rt_table, char *ip_addr, char mask){
    char dst_str_with_mask[16];
    apply_mask(ip_addr,mask,dst_str_with_mask);
    l3_route_t *l3_route = rt_table_lookup(rt_Table,dst_str_with_mask,mask);
    if(!l3_route){
        return;
    }

    //found route and delete it
    remove_glthread(&l3_route->rt_glue);
    free(l3_route);
}














static bool_t *_rt_table_entry_add(rt_table_t *rt_table, l3_route_t *l3_route){
    //first check if route already exists
    l3_route_t *l3_route_old = rt_table_lookup(rt_table,l3_route->dest,l3_route->mask);

    //if old route is equal to the new route return false and dont do anything
    if(l3_route_old && IS_L3_ROUTES_EQUAL(l3_route_old, l3_route)){
        return FALSE;
    }

    //delete old entry and add in new entry
    if(l3_route_old){
        delete_rt_table_entry(rt_table,l3_route_old->dest,l3_route_old->mask);

    }

    //or just add the new entry if it doesnt already exist

    init_glthread(&l3_route->rt_glue);
    glthread_add_next(&rt_table->route_list, &l3_route->rt_glue);
    return TRUE;


}



void rt_table_add_direct_route(rt_table_t *rt_table, char *dst, char mask){
    //no oif or gateway IP so call add route fcn with 0 passed for those
    rt_table_add_route(rt_table,dst,mask,0,0);
}





void rt_table_add_route(rt_table_t *rt_Table, char *dst, char mask, char *gw, char *oif){
    unsigned int dst_int;
    char dest_str_with_mask[16];
    apply_mask(dst,mask,dest_str_with_mask);
    inet_pton(AF_INET,dst_str_with_msk,&dst_int);

    //dest route is the subnet ID which is why we apply mask


    l3_route_t *l3_route - l3rib_lookup_lpm(rt_table,dst_int);

    //init mem
    l3_route = calloc(1,sizeof(l3_route_t));

    //copy the subnet ID into the dest field of the route
    strncpy(l3_route->dest,dest_str_with_mask,16);
    //esc char
    l3_route->dest[15] = '\0';

    //set mask field for route
    l3_route->mask = mask;

    //set the is direct field
    if(!gw && !oif){
        l3_route->is_direct = TRUE;
    }
    else{
        l3_route->is_direct = FALSE;
    }


    if(gw && oif){
        //copy gateway ip field into l3 route
        strncpy(l3_route->gw_ip,gw,16);
        //esc char
        l3_route->gw_ip[15] = '\0';

        //copy the oif field into l3 route
        strncpy(l3_route->oif,oif,IF_NAME_SIZE);
        l3_route->oif[IF_NAME_SIZE -1] = '\0';
    }

    //add route into table
    if(! _rt_table_entry_add(rt_table,l3_route)){
        printf("Error : Route %s/%d Instantiation Failed \n",
            dst_str_with_mask,mask);
        free(l3_route);
    }


}












void
dump_rt_table(rt_table_t *rt_table){

    glthread_t *curr = NULL;
    l3_route_t *l3_route = NULL;

    printf("L3 Routing Table:\n");
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list, curr){

        l3_route = rt_glue_to_l3_route(curr);
        printf("\t%-18s %-4d %-18s %s\n", 
                l3_route->dest, l3_route->mask,
                l3_route->is_direct ? "NA" : l3_route->gw_ip, 
                l3_route->is_direct ? "NA" : l3_route->oif);

    } ITERATE_GLTHREAD_END(&rt_table->route_list, curr); 
}




