/*
 * =====================================================================================
 *
 *       Filename:  comm.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-12-08 06:48:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __COMM__
#define __COMM__


#define MAX_PACKET_BUFFER_SIZE  2048

typedef struct node_ node_t;
typedef struct interface_ interface_t;

int send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *interface);

int pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size);

int send_pkt_flood(node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size);

#endif
