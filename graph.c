/*
 * =====================================================================================
 *
 *       Filename:  graph.c
 *
 *    Description: Defining the graph data structure used for node topology
 *
 *        Version:  1.0
 *        Created:  20-11-27 06:37:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>


graph_t * create_new_graph(char *topology_name){
    //use calloc to initialize 1 item memory to 0
    graph_t *graph = calloc(1,sizeof(graph_t));
    //use strncpy to copy topology name to graph struct field
    strncpy(graph->topology_name,topology_name,32);
    //0 ending
    graph->topology_name[32] = '\0';
    //intialize linked list
    init_glthread(&graph->node_list);
    graph->gstdout = FALSE;
    return graph;
}


node_t * create_graph_node(graph_t *graph, char *node_name){
    //create new data structure of type node
    node_t *node = calloc(1,sizeof(node_t));
    //copy node_name in parameter to the struct field
    strncpy(node->node_name,node_name,NODE_NAME_SIZE);
    node->node_name[NODE_NAME_SIZE]='\0';
   
   //assign port number and socket to node on creation
    init_udp_socket(node);
    //initialize node networking info

    //TODO: check if this causes program crash
    node->spf_data = NULL;
    init_node_nw_prop(&node->node_nw_prop);


    //LOGGING INIT
    tcp_ip_init_node_log_info(node);

    init_glthread(&node->graph_glue); 
    //add the glthread node to the linked list
    glthread_add_next(&graph->node_list,&node->graph_glue);
    return node;
}


void
insert_link_between_two_nodes(node_t *node1,
        node_t *node2,
        char *from_if_name,
        char *to_if_name,
        unsigned int cost){

    link_t *link = calloc(1, sizeof(link_t));

    /*Set interface properties*/
    strncpy(link->intf1.if_name, from_if_name, IF_NAME_SIZE);
    link->intf1.if_name[IF_NAME_SIZE - 1] = '\0';
    strncpy(link->intf2.if_name, to_if_name, IF_NAME_SIZE);
    link->intf2.if_name[IF_NAME_SIZE - 1] = '\0';
    
    link->intf1.link= link; /*set back pointer to link*/
    link->intf2.link= link; /*set back pointer to link*/

    link->intf1.att_node = node1;
    link->intf2.att_node = node2;
    link->cost = cost;

    int empty_intf_slot;

    /*Plugin interface ends into Node*/
    empty_intf_slot = get_node_intf_available_slot(node1);
    node1->intf[empty_intf_slot] = &link->intf1;

    empty_intf_slot = get_node_intf_available_slot(node2);
    node2->intf[empty_intf_slot] = &link->intf2;

    init_intf_nw_prop(&link->intf1.intf_nw_props);
    init_intf_nw_prop(&link->intf2.intf_nw_props);

    /*Now Assign Random generated Mac address to the Interfaces*/
    interface_assign_mac_address(&link->intf1);
    interface_assign_mac_address(&link->intf2);

    //init interface logging
    tcp_ip_init_intf_log_info(&link->intf1);
    tcp_ip_init_intf_log_info(&link->intf2);
}

void dump_graph(graph_t *graph){
     //print all graph info
     node_t *node;
     glthread_t *curr;
     //print name of graph
     printf("Topology name = %s\n",graph->topology_name);
     //iterate through each node in linked list and print their information
     ITERATE_GLTHREAD_BEGIN(&graph->node_list,curr){
         node = graph_glue_to_node(curr);
         dump_node(node);
     }ITERATE_GLTHREAD_END(&graph->node_list,curr);

}


void dump_node(node_t *node){
    //print node info
    interface_t *intf;
    unsigned int i =0;
    printf("Node Name = %s : \n",node->node_name);

    //loop through all interfaces
    for (;i<MAX_INTF_PER_NODE;i++){
        //check if there is an interface
        intf = node->intf[i];
        if(!intf) break;
        //if we are here we have an interface
        dump_interface(intf);
    }

}

void dump_interface(interface_t *interface){

   link_t *link = interface->link;
   node_t *nbr_node = get_nbr_node(interface);

   printf("Interface Name = %s\n\tNbr Node %s, Local Node : %s, cost = %u\n", 
            interface->if_name,
            nbr_node->node_name, 
            interface->att_node->node_name, 
            link->cost);
}



