/*
 * =====================================================================================
 *
 *       Filename:  comm.h
 *
 *    Description:  File for function which define communication between nodes
 *
 *        Version:  1.0
 *        Created:  20-12-08 06:48:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __COMM__
#define __COMM__

#include <stdint.h>

#define MAX_PACKET_BUFFER_SIZE  2048

typedef struct node_ node_t;
typedef struct interface_ interface_t;



/* 
 *This Function sends a packet out of a specific interface.
 *The neighbour noe must receive packet at end of link
 * */
int send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *interface);

/*
 * This function is for receiving a packet out of the interface
 */
int pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size);

/*
 * This function is used to flood the packet out of all interfaces of the node
 */
int send_pkt_flood(node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size);

/*
 * This function is used to flood the packet out of all L2 interfaces of the node
 */
int send_pkt_flood_l2_intf_only(node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size);
#endif
