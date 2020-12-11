/*
 * =====================================================================================
 *
 *       Filename:  layer2.h
 *
 *    Description:  Defining ethernet header
 *
 *        Version:  1.0
 *        Created:  20-12-10 06:25:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: Chris Devito 
 *   Organization:  
 *
 * =====================================================================================
 */


/*  PRAGMA IS USED TO ENSURE COMPILER DOES NOT ADD ANY PADDING BYTES TO ANY OF THE DEFINED FIELDS */

#ifndef __LAYER2__
#define __LAYER2__

#include "../net.h"
#include "../gluethread/glthread.h"

#pragma pack(push,1)
typedef struct ethernet_hdr_{
    mac_add_t src_mac; //6 bytes
    mac_add_t dest_mac; //6 bytes
    char payload[248]; //max 1500 bytes but chose smaller
    unsigned int FCS; //CRC field - frame check sequence
    short type; //2 bytes
}ethernet_hdr_t;
#pragma pack(pop)




//MACRO DEFINED TO EVALUATE THE SIZE OF THE ETHERNET HEADER EXCLUDING THE PAYLOAD
//what this does is it gets the size of the entire ethernet header struct and subtracts the payload size and returns
#define ETH_HDR_SIZE_EXCL_PAYLOAD   \
    (sizeof(ethernet_header_t) - sizeof(((ethernet_hdr_t *)0)->payload))

//MACRO DEFINED TO RETURN THE FCS VALUE PRESENT IN THE FRAME
//what this does it it gets the location of the start of the payload, then adds it to the payload size to get the end
//at the end of the payload size is the 4 bytes of FCS so we cast to an unsigned int and return
#define ETH_FCS(eth_hdr_ptr,payload_size)   \
    (*(unsigned int)(((char *)(((ethernet_hdr_t *)eth_hdr_t->payload)+payload_size)))

//static function to allocate ethernet header with payload packet
static ethernet_hdr_t * ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    //must encapsulate data into payload of ethernet header
    //initialize all fields of ethernet header to zero also
    
    char *temp = calloc(pkt,pkt_size);
    memcpy(temp,pkt,pkt_size);

    ethernet_hdr_t *header = (ethernet_hdr_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    //header is now pointing to where the payload data should be written to in the ethernet header
    memset(header,0,ETH_HDR_SIZE_EXCL_PAYLOAD);
    //above line sets all fields before the payload to be zero
    memcpy(header->payload,temp,pkt_size);
    free(temp);
    //set the FCS to be zero
    ETH_FCS(header,pkt_size) = 0;    

}





#endif
