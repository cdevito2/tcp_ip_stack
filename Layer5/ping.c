/*
 * =====================================================================================
 *
 *       Filename:  layer5.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-13 12:06:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include "../graph.h"
#include "../Layer3/layer3.h"
#include "../gluethread/glthread.h"
#include "tcpconst.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /*for AF_INET*/
#include <arpa/inet.h>  /*for inet_pton and inet_ntop*/


extern void demote_packet_to_layer3(node_t *node,char *pkt, unsigned int size,
            int protocol_number, unsigned int dest_ip_address);

void layer5_ping_fn(node_t *node, char *dst_ip_addr){
    unsigned int addr_int;
    printf("Src Node : %s, Ping IP : %s\n",node->node_name,dst_ip_addr);
    inet_pton(AF_INET,dst_ip_addr,&addr_int);
    addr_int = htonl(addr_int);

    //send to L3
    demote_packet_to_layer3(node, NULL, 0, ICMP_PRO, addr_int);
}
