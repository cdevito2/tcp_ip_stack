/*
 * =====================================================================================
 *
 *       Filename:  net.h
 *
 *    Description:  has structs needed for network programming
 *
 *        Version:  1.0
 *        Created:  20-11-29 07:52:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */
#include <memory.h>
#include "utils.h"
#ifndef __NET__
#define __NET__

typedef struct graph_ graph_t;
typedef struct node_ node_t;
typedef struct interface_ interface_t;
typedef struct arp_table_ arp_table_t;
#pragma pack(push,1)
typedef struct ip_addr_{
    char ip_addr[16]; //maximum of 15 characters
}ip_addr_t;

typedef struct mac_addr_{
    char mac[48];
}mac_addr_t;

typedef struct node_nw_prop_{
    bool_t is_lb_addr_config;//bool to see if loopback configured
    ip_addr_t lb_addr; //loopback address
    arp_table_t *arp_table;
}node_nw_prop_t;
#pragma pack(pop)
extern void init_arp_table(arp_table_t **arp_table);

//function to initia;ize network properties of node
static inline void init_node_nw_prop(node_nw_prop_t *node_nw_prop){
    /*  default init */
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr,0,16);
    init_arp_table(&(node_nw_prop->arp_table));

}

typedef struct intf_nw_props{
    //every interface needs a mac address
    mac_addr_t mac_add;
    //interface may have an ip address
    ip_addr_t ip_add;
    //interface ip needs a mask value
    char mask;
    //flag to see if ip address configured
    bool_t is_ipadd_config;

}intf_nw_props_t;


//function to initialize network properties of interface
static inline void init_intf_nw_prop(intf_nw_props_t *intf_nw_props){
    /* default init */
    intf_nw_props->mask =0;
    memset(intf_nw_props->ip_add.ip_addr,0,16);
    memset(intf_nw_props->mac_add.mac,0,48);
    intf_nw_props->is_ipadd_config = FALSE;

}

//create some shorthand macros

#define IF_MAC(intf_ptr)    ((intf_ptr)->intf_nw_props.mac_add.mac)
#define IF_IP(intf_ptr)     ((intf_ptr)->intf_nw_props.ip_add.ip_addr)
#define NODE_LO_ADDR(node_ptr)      ((node_ptr)->node_nw_prop.lb_addr.ip_addr)
#define IS_INTF_L3_MODE(intf_ptr)   ((intf_ptr)->intf_nw_prop.is_ipadd_config == TRUE)

//macro to return true if interface is in L3 mode or not, false if in L2 mode




//functions to define in net.c

unsigned int ap_addr_p_to_n(char *ip_addr);
void ip_addr_n_to_p(unsigned int ip_addr, char *ip_addr_str);
interface_t * node_get_matching_subnet_interface(node_t *node, char *ip_addr);
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer);
unsigned int convert_ip_from_str_to_int(char *str_ip_address);
bool_t node_set_loopback_address(node_t *node, char *ip_addr);
bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask);
bool_t node_unset_intf_ip_address(node_t *node, char *local_if);
void interface_assign_mac_address(interface_t *interface);
void dump_nw_graph(graph_t *graph);
char * pkt_buffer_shift_right(char *pkt, unsigned int pkt_size, unsigned int total_buffer_size);
#endif
