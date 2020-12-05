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
#include "utils.c"
#include <stdlib.h>
#include <netinet/in.h>

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


interface_t * node_get_matching_subnet_interface(node_t *node, char *ip_addr){
    //step 1 is to find the subnet for the passed in ip address
    char mask;
    char subnet[16];
    char sub2[16];
    unsigned int i = 0;
    interface_t *intf;
    char *intf_ip = NULL;
    //allocate the space for the resulting subnets
    memset(subnet,0,16);
    memset(sub2,0,16);
    //loop through all interfaces on node
    for(i=0;i<MAX_INTF_PER_NODE;i++){
        intf = node->intf[i];
        //ensure we are at a valid interface
        if(!intf){
            return NULL;
        }
        //if we are here then we have a valid interface, so check if it has assigned ip 
        if(intf->intf_nw_props.is_ipadd_config == FALSE){
            continue;
        }
        intf_ip = IF_IP(intf);
        //find mask
        mask = intf->intf_nw_props.mask;
        //use utils.c function which determines the subnet id
        apply_mask(intf_ip,mask,subnet);
        //do the same for the ip passed in as arg
        apply_mask(ip_addr,mask,sub2);
        //compare the resulting char buffers
        if(strcmp(subnet,sub2) == 0){
            return intf;
        }


    }
}

//funciton to convert ip from int form into A.B.C.D/x string form
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){
    //call the inet_ntop function
    inet_ntop(AF_INET,&ip_addr,output_buffer,16);
    output_buffer[15] = '\0';
}

//function to convert ip from str(A.B.C.D/x) into decimal form
unsigned int convert_ip_from_str_to_int(char *str_ip_addr){
    unsigned int ip = 0;
    //use inet_pton and htonl to handle endianess
    inet_pton(AF_INET,str_ip_addr,&ip);
    ip = htonl(ip);
    return ip;
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








