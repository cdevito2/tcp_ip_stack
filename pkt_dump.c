/*
 * =====================================================================================
 *
 *       Filename:  pkt_dump.c
 *
 *    Description:  Packet dump function
 *
 *        Version:  1.0
 *        Created:  21-01-26 05:19:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#include "Layer2/layer2.h"
#include "tcpconst.h"
#include <stdio.h>

void pkt_dump(ethernet_hdr_t *ethernet_hdr, unsigned int pkt_size){
    //print necessary headers of the pkt including
    //Eth hdr, Arp hdr, IP hdr, for unknown payload type print offset and size of payload in frame


    //tip write a seperate function to print each type of hdr

}

