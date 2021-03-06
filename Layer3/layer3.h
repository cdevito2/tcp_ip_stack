/*
 * =====================================================================================
 *
 *       Filename:  layer3.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-07 12:45:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __LAYER3__
#define __LAYER3__

#include <stdint.h>
#include "../tcpconst.h"

//IP header 
//total length constant 20 byte
#pragma pack(push,1)
typedef struct ip_hdr_{
    unsigned int version : 4; //4 bits for IPv4
    unsigned int ihl : 4;//4 bits, length of IP hdr, standard value is 5 which = 20 bytes
    char tos; //type of service - 8 bits
    unsigned short total_length; //length of header plus payload
    unsigned short identification;//not using
    unsigned int unused_flag : 1;//not using
    unsigned int DF_flag : 1;//not using
    unsigned int MORE_flag : 1;//not using
    unsigned int frag_offset : 13;//not using

    char ttl;//8bit-using
    char protocol;//8bit-using
    unsigned short checksum;//16 bit field not using
    unsigned int src_ip;//32 bit
    unsigned int dst_ip;//32 bit

}ip_hdr_t;
#pragma pack(pop)

static inline void initialize_ip_hdr(ip_hdr_t *ip_hdr){
    //set version field to 4
    ip_hdr->version = 4;

    //set header length to 5 to represent 20 bytes
    ip_hdr->ihl = 5;

    //not using tos
   ip_hdr->tos=0;
   ip_hdr->total_length=0;//this will be overwritten when header is modified


    /*  init unused fields to 0 */
    ip_hdr->identification=0;
    ip_hdr->unused_flag=0;
    ip_hdr->DF_flag=1;
    ip_hdr->MORE_flag=0;
    ip_hdr->frag_offset=0;

    ip_hdr->ttl=64;
    ip_hdr->protocol=0;//this must be overwritten when header is modified
    ip_hdr->checksum =0;//not used
    ip_hdr->src_ip=0;
    ip_hdr->dst_ip=0;


}


#define IP_HDR_LEN_BYTES(ip_hdr_ptr)    (ip_hdr_ptr->ihl * 4)
#define IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr_ptr)   (ip_hdr_ptr->total_length * 4)
#define INCREMENT_IPHDR(ip_hdr_ptr)     ((char *)ip_hdr_ptr + (ip_hdr_ptr->ihl *4))
#define IP_HDR_PAYLOAD_SIZE(ip_hdr_ptr) (IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr_ptr) - \
        IP_HDR_LEN_BYTES(ip_hdr_ptr))   




typedef struct rt_table_{
    glthread_t route_list;
}rt_table_t;

typedef struct nexthop_{
    char gw_ip[16];
    interface_t *oif;
    uint32_t ref_count;
}nexthop_t;


#define nexthop_node_name(nexthop_ptr)  \
   ((get_nbr_node(nexthop_ptr->oif))->node_name)

typedef struct l3_route_{
    char dest[16];
    char mask;
    bool_t is_direct;//true = no gateway or outgoing interface
    nexthop_t *nexthops[MAX_NXT_HOPS];
    uint32_t spf_metric;
    int nxthop_idx;
    glthread_t rt_glue;
}l3_route_t;
GLTHREAD_TO_STRUCT(rt_glue_to_l3_route,l3_route_t,rt_glue);

nexthop_t *l3_route_get_active_nexthop(l3_route_t *l3_route);

void init_rt_table(rt_table_t **rt_table);


void rt_table_add_route(rt_table_t *rt_table, char *dst, char mask, char *gw, interface_t *oif,uint32_t spf_metric);

void rt_table_add_direct_route(rt_table_t *rt_table, char *dst, char mask);

void dump_rt_table(rt_table_t *rt_table);

void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size, int L3_protocol_number);



void demote_packet_to_layer3(node_t *node, char *pkt, unsigned int size, int protocol_number, unsigned int dest_ip_address);

void
clear_rt_table(rt_table_t *rt_table);

l3_route_t *
rt_table_lookup(rt_table_t *rt_table, char *ip_addr, char mask);


l3_route_t *
l3rib_lookup(rt_table_t *rt_table, uint32_t dest_ip, char mask);

l3_route_t *l3rib_lookup_lpm(rt_table_t *rt_table, unsigned int dest_ip);


#define IS_L3_ROUTES_EQUAL(rt1, rt2)              \
    ((strncmp(rt1->dest, rt2->dest, 16) == 0) &&  \
    (rt1->mask == rt2->mask) &&                   \
    (rt1->is_direct == rt2->is_direct) &&         \
    (strncmp(rt1->gw_ip, rt2->gw_ip, 16) == 0) && \
    strncmp(rt1->oif, rt2->oif, IF_NAME_SIZE) == 0)
#endif 
