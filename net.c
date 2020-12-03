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
    interface_t *intf = get_node_if_by_name(node,local_if);
    //now we have the interface struct for the node interface we want to apply ip 
    strncpy(IF_IP(intf),ip_addr,16);
    IF_IP(intf)[16]='\0';
    intf->intf_nw_props.mask = mask;
    intf->intf_nw_props.is_ipadd_config = TRUE;
    return TRUE;
}


void dump_node_nw_props(node_t *node){
    printf("\nNode Name = %s\n", node->node_name);
    if(node->node_nw_prop.is_lb_addr_config){
        printf("\t lo addr: %s/32\n",NODE_LO_ADDR(node));
    }
}


void dump_intf_props(interface_t *interface){
    dump_interface(interface);
    if(interface->intf_nw_props.is_ipadd_config){
        printf("\t IP ADDR = %s/%u",IF_IP(interface),interface->intf_nw_props.mask);
    }
    else{
        printf("\t IP ADDR = %s/%u","Nil",0);
    }

    //now print mac addresses
    printf("\t MAC: %u:%u:%u:%u:%u:%u\n",IF_MAC(interface)[0],IF_MAC(interface)[1],IF_MAC(interface)[3],IF_MAC(interface)[4],IF_MAC(interface)[5]);

}


void dump_nw_graph(graph_t *graph){
    node_t *node;
    glthread_t *curr;
    interface_t *interface;
    printf("Topology Name = %s\n",graph->topology_name);
    unsigned int i;
    ITERATE_GLTHREAD_BEGIN(&graph->node_list,curr){
        node = graph_glue_to_node(curr);
        dump_node_nw_props(node);
        for (i=0;i<MAX_INTF_PER_NODE;i++){
            interface = node->intf[i];
            if(!interface){
                break;
            }
            dump_intf_props(interface);
        }
    }ITERATE_GLTHREAD_END(&graph->node_list,curr);


}








