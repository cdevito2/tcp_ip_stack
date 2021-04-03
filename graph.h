/*
 * =====================================================================================
 *
 *       Filename:  graph.h
 *
 *    Description: Defining the graph data structure used for node topology  
 *
 *        Version:  1.0
 *        Created:  20-11-27 06:37:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __GRAPH__
#define __GRAPH__

#include "gluethread/glthread.h" 
#include "net.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include "tcp_ip_trace.h"
     
#define NODE_NAME_SIZE 16
#define IF_NAME_SIZE 16
#define MAX_INTF_PER_NODE 10

/* Forward declarations to prevent recursive dependency compilation error */
typedef struct node_ node_t;
typedef struct link_ link_t;


//using typedef to give a name to my defined structures

//graph is a collection of nodes
// each node has 1 or more interfaces
// each interface has a name, an owning node, and a link
// each node has a name, and a set of empty interface slots

//typedef identifies graph_t into struct namespace
typedef struct graph_ { 
    char topology_name[32]; //store name of topology
    glthread_t node_list; //modified form of linked list of nodes
    bool_t gstdout;
}graph_t;

typedef struct interface_{
    char if_name[IF_NAME_SIZE]; //name of interface
    struct node_ *att_node; //pointer to the owning node of the interface
    struct link_ *link; //pointer to the link for the interface

    intf_nw_props_t intf_nw_props;//struct to hold networking properties for interface
    log_t log_info;
} interface_t;

typedef struct link_{
    interface_t intf1;
    interface_t intf2;
    unsigned int cost;
}link_t;


static inline uint32_t get_link_cost(interface_t *interface){
    return interface->link->cost;
}


typedef struct spf_data_ spf_data_t;
struct node_{
    char node_name[NODE_NAME_SIZE];
    interface_t *intf[MAX_INTF_PER_NODE];
    glthread_t graph_glue; //used in order to insert into struct graph linked list
    node_nw_prop_t node_nw_prop; //struct to hold networking properties
    unsigned int udp_port_number;
    int udp_sock_fd;
    spf_data_t *spf_data;

    //node logging
    log_t log_info;
};


GLTHREAD_TO_STRUCT(graph_glue_to_node,node_t,graph_glue);
/*  INLINE FUNCTIONS USED AS THEY ARE VERY SMALL FUNCTIONS
 *  static keyword forces compiler to consided this inline function in the linker */
static inline node_t * get_nbr_node(interface_t *interface){
  //return pointer to the neighbor node which is connected to the interface passed in as an argument
  link_t *link = interface->link;
  //grab the link for the given interface
  //check the 2 interfaces on the link and return the one that matches
  if (&link->intf1 == interface){ //we want to find the node connected to the opposite end of the interface passed in as an argument
      //this means that the opposite interface to the one passed in is intf2, so grab the owning node of that interface
      return link->intf2.att_node;
  }
  else{ 
      return link->intf1.att_node;
  }
}
  
static inline interface_t * get_node_if_by_name(node_t *node, char *if_name){
    //given node and interface name return the interface
    //loop through node interfaces and check to see if the names match
    int i;
    interface_t *intf_res;
    for(i=0;i<MAX_INTF_PER_NODE;i++){
        intf_res = node->intf[i];
        if(!intf_res) {
            return NULL;
        }
        if (strncmp(intf_res->if_name, if_name, IF_NAME_SIZE)==0){
            return intf_res;
        }
    }
    return NULL;
}
static inline int get_node_intf_available_slot(node_t *node){
//return index of array with empty available slot into which an interface address could be stored, return -1 if no space
//check if node interface array pointer contains any addresses yet
    int i;
    for (i=0;i<MAX_INTF_PER_NODE;i++){
       //check each slot in the node intf array 
       if(node->intf[i]){
           continue; //the slot is already taken so keep looking
       }
       else{
           //empty slot so return the index
           return i;
       }
   }
   //if we reach here that means there are no available slots so return -1
   return -1;

}

static inline node_t * get_node_by_node_name(graph_t *topo, char *node_name){
    //given graph and node name return the pointer node present in graph list
    //loop through the graph topo node list
    node_t *node;
    glthread_t *curr;
    ITERATE_GLTHREAD_BEGIN(&topo->node_list,curr){
        node=graph_glue_to_node(curr);
        if(strncmp(node->node_name,node_name,NODE_NAME_SIZE)==0){
            return node;
        }
    } ITERATE_GLTHREAD_END(&topo->node_list,curr);
    return NULL;

}

/*  functions to be implement in .c file */

node_t * create_graph_node(graph_t *graph, char *node_name);

void insert_link_between_two_nodes(node_t *node1, node_t *node2, char *from_if_name, char *to_if_name, unsigned int cost);

graph_t * create_new_graph(char *topology_name);

void dump_graph(graph_t *graph);
void dump_node(node_t *node);
void dump_interface(interface_t *interface);

#endif
