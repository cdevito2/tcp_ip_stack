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
#ifndef __NET__
#define __NET__

#include "utils.h"
#include <memory.h>

#define MAX_VLAN_MEMBERSHIP 10

typedef struct graph_ graph_t;
typedef struct node_ node_t;
typedef struct interface_ interface_t;
typedef struct arp_table_ arp_table_t;
typedef struct mac_table_ mac_table_t;
typedef struct rt_table_ rt_table_t;

typedef enum {
    ACCESS,
    TRUNK,
    L2_MODE_UNKNOWN,
}intf_l2_mode_t;


//funciton to translate enum to string
static inline char * intf_l2_mode_str(intf_l2_mode_t intf_l2_mode){
    switch(intf_l2_mode){
        case ACCESS:
            return "access";
        case TRUNK:
            return "trunk";
        default:
            return "L2_MODE_UNKNOWN";
    }
}



#pragma pack(push,1)
typedef struct ip_add_{
    unsigned char ip_addr[16]; //maximum of 15 characters
}ip_add_t;

typedef struct mac_add_{
    unsigned char mac[6];
}mac_add_t;
#pragma pack(pop)

typedef struct node_nw_prop_{
    
    arp_table_t *arp_table;
    
    mac_table_t *mac_table;

    rt_table_t *rt_table;

    bool_t is_lb_addr_config;//bool to see if loopback configured
    
    ip_add_t lb_addr; //loopback address
}node_nw_prop_t;


extern void init_arp_table(arp_table_t **arp_table);
extern void init_mac_table(mac_table_t **mac_table);
extern void init_rt_table(rt_table_t **rt_table);


//function to initia;ize network properties of node
static inline void init_node_nw_prop(node_nw_prop_t *node_nw_prop){
    /*  default init */
    node_nw_prop->is_lb_addr_config = FALSE;
    memset(node_nw_prop->lb_addr.ip_addr,0,16);
    init_arp_table(&(node_nw_prop->arp_table));
    init_mac_table(&(node_nw_prop->mac_table));
    init_rt_table(&(node_nw_prop->rt_table));
}




typedef struct intf_nw_props{

    bool_t is_up;
    //every interface needs a mac address
    mac_add_t mac_add;
    intf_l2_mode_t intf_l2_mode; //if ip address configured this is set to unknown
    //interface may have an ip address
    ip_add_t ip_add;
    //interface ip needs a mask value
    char mask;
    //flag to see if ip address configured
    bool_t is_ipadd_config;
    unsigned int vlans[MAX_VLAN_MEMBERSHIP];

}intf_nw_props_t;




//function to initialize network properties of interface
static inline void init_intf_nw_prop(intf_nw_props_t *intf_nw_props){
    /* default init */
    intf_nw_props->mask =0;
    intf_nw_props->is_up = TRUE;
    memset(intf_nw_props->ip_add.ip_addr,0,16);
    memset(intf_nw_props->mac_add.mac,0,sizeof(intf_nw_props->mac_add.mac));
    memset(intf_nw_props->vlans,0,sizeof(intf_nw_props->vlans));
    intf_nw_props->is_ipadd_config = FALSE;

}




//create some shorthand macros

#define IF_IS_UP(intf_ptr)  ((intf_ptr)->intf_nw_props.is_up == TRUE)
#define IF_MAC(intf_ptr)    ((intf_ptr)->intf_nw_props.mac_add.mac)
#define IF_IP(intf_ptr)     ((intf_ptr)->intf_nw_props.ip_add.ip_addr)
#define NODE_LO_ADDR(node_ptr)      ((node_ptr)->node_nw_prop.lb_addr.ip_addr)
#define IS_INTF_L3_MODE(intf_ptr)   (intf_ptr->intf_nw_props.is_ipadd_config == TRUE)
#define NODE_ARP_TABLE(node_ptr)    (node_ptr->node_nw_prop.arp_table)
#define NODE_MAC_TABLE(node_ptr)     (node_ptr->node_nw_prop.mac_table)
#define NODE_RT_TABLE(node_ptr)     (node_ptr->node_nw_prop.rt_table)
#define IF_L2_MODE(intf_ptr)    (intf_ptr->intf_nw_props.intf_l2_mode)       
//macro to return true if interface is in L3 mode or not, false if in L2 mode




//functions to define in net.c
interface_t * node_get_marching_subnet_interface(node_t *node, char *ip_addr);
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
unsigned int get_access_intf_operation_vlan_id(interface_t *interface);
bool_t is_trunk_interface_vlan_enabled(interface_t *interface, unsigned int vlan_id);
#endif
