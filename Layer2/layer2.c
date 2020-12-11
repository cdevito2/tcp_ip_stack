/*
 * =====================================================================================
 *
 *       Filename:  layer2.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-12-10 07:49:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "graph.h"


void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    //entry point into TCP/IP stack from the bottom
}

void init_arp_table( arp_table_t **arp_table){
    //calloc arp table
    calloc(1,sizeof(arp_table_t));
    //init linked list which will hold pointer to arp table entries
    init_glthread(&((*arp_table)->arp_entries));
}


