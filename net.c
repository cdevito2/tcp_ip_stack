/*
 * =====================================================================================
 *
 *       Filename:  net.c
 *
 *    Description:  networking functions
 *
 *        Version:  1.0
 *        Created:  20-12-01 08:09:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chris devito
 *   Organization:  
 *
 * =====================================================================================
 */
#include "net.h"
#include "graph.h"
#include <stdio.h>
#include <utils.h>



void interface_assign_mac_address(interface_t *interface){
    //assigne mac address to a given mac address
    memset(IF_MAC(interface),0,48);
    strcpy(IF_MAC(interface),interface->att_node->node_name);
    strcat(IF_MAC(interface),interface->if_name);
}



bool_t node_set_loopback_address(node_t *node, char *ip_addr){
    node->node_nw_prop.is_lb_addr_config = TRUE;
    //assign ip
    strncpy(NODE_LO_ADDR(node),ip_addr,16);
    NODE_LO_ADDR(node)[16] = '\0';
    return TRUE;
}

bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask){
    interface_t *intf = get_node_by_if_name(node,local_if);
    //now we have the interface struct for the node interface we want to apply ip 
    strncpy(IF_IP(intf),ip_addr,16);
    IF_IP(intf)[16]='\0';
    intf->intf_nw_props.mask = mask;
    intf->intf_nw_props.is_ipadd_config = TRUE;
    return TRUE;
}

