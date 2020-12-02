/*
 * =====================================================================================
 *
 *       Filename:  topologies.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-11-27 07:22:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "graph.h"

graph_t * build_first_topo(){
    //implement generic graph



    //create new graph
    graph_t *topo = create_new_graph("Hello World Generic Graph");
    //construct nodes of the graph
    node_t *R0_re = create_graph_node(topo, "R0_re");
    node_t *R1_re = create_graph_node(topo, "R1_re");
    node_t *R2_re = create_graph_node(topo, "R2_re");

    //insert the links between nodes
    insert_link_between_two_nodes(R0_re,R1_re, "eth0/0","eth0/1",1);
    insert_link_between_two_nodes(R1_re,R2_re, "eth0/2","eth0/3",1);
    insert_link_between_two_nodes(R0_re,R2_re, "eth0/4","eth0/5",1);
    
    //assign loopback address
    node_set_loopback_address(R0_re,"122.1.1.0");
    //return pointer to graph
    node_set_intf_ip_address(R0_re,"eth0/4","40.1.1.1",24);
    node_set_intf_ip_address(R0_re,"eth0/0","20.1.1.1",24);

    
    //assign loopback address
    node_set_loopback_address(R1_re,"122.1.1.1");
    //return pointer to graph
    node_set_intf_ip_address(R1_re,"eth0/1","20.1.1.2",24);
    node_set_intf_ip_address(R1_re,"eth0/2","30.1.1.1",24);
    //assign loopback address
    node_set_loopback_address(R2_re,"122.1.1.2");
    //return pointer to graph
    node_set_intf_ip_address(R2_re,"eth0/3","30.1.1.2",24);
    node_set_intf_ip_address(R2_re,"eth0/5","40.1.1.2",24);

   return topo; 
}
