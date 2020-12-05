/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description:  utility functions
 *
 *        Version:  1.0
 *        Created:  20-12-02 06:58:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#include "utils.h"
#include <arpa/inet.h>
#include <sys/socket.h>

void layer2_fill_with_broadcast_mac(char *mac_array){
    mac_array[0] = 0xFF;
    mac_array[1] = 0xFF;
    mac_array[2] = 0xFF;
    mac_array[3] = 0xFF;
    mac_array[4] = 0xFF;
    mac_array[5] = 0xFF;
}



void apply_mask(char *prefix, char mask, char *str_prefix){

    unsigned int ip_addr_int = 0;
    //convert ip to bytes, ex 192.168.251.9 = some decimal bnum
    inet_pton(AF_INET,prefix,&ip_addr_int);
    ip_addr_int = htonl(ip_addr_int);
    //figure out the subnet mask
    unsigned int subnet_mask = 0xFFFFFFFF;
    //if mask is 24, then for below trail bits wouldbe 8
    char trail_bits = subnet_mask - mask;

    subnet_mask = subnet_mask << trail_bits;

    //do a bitwise AND to determine network id
    ip_addr_int = ip_addr_int & subnet_mask;
    ip_addr_int = htonl(ip_addr_int);
    inet_ntop(AF_INET,&ip_addr_int,str_prefix,16);
    str_prefix[15]='\0';


}
