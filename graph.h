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
#include "net.h"
#include "gluethread/glthread.h" 
#include <assert.h>

     
#define NODE_NAME_SIZE 16
#define IF_NAME_SIZE 32
#define MAX_INTF_PER_NODE 10

/* Forward declarations */
typedef struct node_ node_t;
typedef struct link_ link_t


//using typedef to give a name to my defined structures

//graph is a collection of nodes
// each node has 1 or more interfaces
// each interface has a name, an owning node, and a link
// each node has a name, and a set of empty interface slots


typedef struct graph_{ //this line defined the identifies graph_ into the struct namespace
    char topology_name[32]; //store name of topology
    glthread_t node_list; //modified form of linked list of nodes

};

typdef struct interface_{
    char if_name[IF_NAME_SIZE]; //name of interface
    struct node_ *att_node; //pointer to the owning node of the interface
    struct link_ *link; //pointer to the link for the interface

} interface_t;

typedef struct link_{
    interface_t intf1;
    interface_t intf2;
    unsigned int cost;
};

struct node_{
    char node_name[NODE_NAME_SIZE];
    interface_t *intf[MAX_INTF_PER_NODE];
    glthread_t graph_glue; //used in order to insert into struct graph linked list
};


GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t,graph_glue);
/*  INLINE FUNCTIONS USED AS THEY ARE VERY SMALL FUNCTIONS
 *  static keyword forces compiler to consided this inline function in the linker */
static inline node_t * get_nbr_node(interface_t *interface){
  //return pointer to the neighbor node which is connected to the interface passed in as an argument
  link_t link = interface->link;
  //grab the link for the given interface
  //check the 2 interfaces on the link and return the one that matches
  if (&link->intf1 == interface){ //we want to find the node connected to the opposite end of the interface passed in as an argument
      //this means that the opposite interface to the one passed in is intf2, so grab the owning node of that interface
      return link->intf2.att_node;
  }
  else{ 
      return link->intf1.att_node
  }
}
  

static inline int get_node_intf_available_slot(node_t *node){
//return index of array with empty available slot into which an interface address could be stored, return -1 if no space
//check if node interface array pointer contains any addresses yet
   for (int i=0;i<MAX_INTF_PER_NODE;i++){
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


/*  functions to be implement in .c file */

graph_t * create_graph_node(graph_t *graph, char *node_name);

void insert_link_between_two_nodes(node_t *node1, node_t *node2, char *from_if_name, char *to_if_name, unsigned int cost);

graph_t * create_new_graph(char *topology_name);




