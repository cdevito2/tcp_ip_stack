/*
 * =====================================================================================
 *
 *       Filename:  tcp_ip_trace.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-03-13 11:20:04 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __TCP_IP_TRACE__
#define __TCP_IP_TRACE__

#include <stdio.h>

typedef struct node_ node_t;
typedef struct interface_ interface_t;

typedef enum{
    ETH_HDR,
    IP_HDR
}hdr_type_t;

#define TCP_PRINT_BUFFER_SIZE 1024

//log file data struct

typedef struct log_{
    bool_t all;
    bool_t recv;
    bool_t send;
    bool_t is_stdout;
    bool_t l3_fwd;
    FILE *log_file;
}log_t;


void tcp_ip_init_node_log_info(node_t *node);
void tcp_ip_init_intf_log_info(interface_t *intf);

void tcp_dump_recv_logger(node_t *node, interface_t *intf, char *pkt, uint32_t pkt_size, hdr_type_t hdr_type);

void tcp_dump_send_logger(node_t *node, interface_t *intf, char *pkt, uint32_t pkt_size, hdr_type_t hdr_type);


#endif
