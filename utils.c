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
#include <string.h>
#include <stdint.h>



/*  void apply_mask(char *prefix, char mask, char *str_prefix){

    printf("IP ADDR IN APPLY MASK FCN : %s\n",prefix);
    printf("MASK VALUE : %u\n",mask);

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

    printf("APPLY MASK RETURN : %s\n",str_prefix);


}*/

void
apply_mask(char *prefix, char mask, char *str_prefix){

    unsigned int binary_prefix = 0, i = 0;
    inet_pton(AF_INET, prefix, &binary_prefix);
    binary_prefix = htonl(binary_prefix);
    for(; i < (32 - mask); i++)
        UNSET_BIT(binary_prefix, i);
    binary_prefix = htonl(binary_prefix);
    inet_ntop(AF_INET, &binary_prefix, str_prefix, 16);
    str_prefix[15] = '\0';
}

char * tcp_ip_convert_ip_n_to_p(uint32_t ip_addr, char *output_buffer){
    char *output = NULL;
    static char str_ip[16];
    output = !output_buffer ? str_ip : output_buffer;
    memset(output,0,16);
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET,&ip_addr,output,16);
    output[15] = '\0';
    return output;

}

uint32_t tcp_ip_convert_ip_p_to_n(char *ip_addr){
    uint32_t binary_prefix = 0;
    inet_pton(AF_INET,ip_addr,&binary_prefix);
    binary_prefix = htonl(binary_prefix);
    return binary_prefix;
}

void layer2_fill_with_broadcast_mac(char *mac_array){
    mac_array[0] = 0xFF;
    mac_array[1] = 0xFF;
    mac_array[2] = 0xFF;
    mac_array[3] = 0xFF;
    mac_array[4] = 0xFF;
    mac_array[5] = 0xFF;
}
