/*
 * =====================================================================================
 *
 *       Filename:  pkt_gen.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-20 06:11:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> /*for struct hostent*/
#include "tcp_public.h"


//dest ip 127.0.0.1
//src is is 127.0.0..1
//protocol is IPPROTO_UDP cuz we sending udp packet
//src port is 0 as it doesnt matter
//dst port is 2001 which is eth2 interface on one of my nodes
//udp payload is a collection of pseudo headers (eth hdr, ip hdr, app hdr)


#define SRC_NODE_UDP_PORT_NO 40000
#define DEST_IP_ADDR "122.1.1.2"
#define INGRESS_INTF_NAME "eth0"

static char send_buffer[MAX_PACKET_BUFFER_SIZE];

static int _send_pkt_out(int sock_fd, char *pkt_data, uint32_t pkt_size, uint32_t dest_udp_port_no){
    int rc;
    struct sockaddr_in dest_addr;
    struct hostent *host = (struct hostent *) gethostbyname("127.0.0.1");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = dest_udp_port_no;
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);

    rc = sendto(sock_fd, pkt_data, pkt_size, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    return rc;
}



int main(int argc, char **argv){
    ///step 1 create UDP socket
    uint32_t n_pkts_send = 0;
    int udp_sock_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

    if(udp_sock_fd == -1){
        printf("Socket Creation failed errno = %d\n",errno);
    }

    //step 2 prepare packet

    memset(send_buffer,0,MAX_PACKET_BUFFER_SIZE);//init buffer

    //provide auxillary info
    strncpy(send_buffer,INGRESS_INTF_NAME,IF_NAME_SIZE);

    //prepare eth hdr
    ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)(send_buffer + IF_NAME_SIZE);
    layer2_fill_with_broadcast_mac(eth_hdr->src_mac.mac);
    layer2_fill_with_broadcast_mac(eth_hdr->dst_mac.mac);
    eth_hdr->type = ETH_IP;
    SET_COMMON_ETH_FCS(eth_hdr,20,0);

    //prepare ip hdr
    ip_hdr_t *ip_hdr = (ip_hdr_t *)(eth_hdr->payload);

    initialize_ip_hdr(ip_hdr);
    ip_hdr->protocol = ICMP_PRO;
    ip_hdr->dst_ip = tcp_ip_convert_ip_p_to_n(DEST_IP_ADDR);
    
    uint32_t total_data_size = ETH_HDR_SIZE_EXCL_PAYLOAD + 20 + IF_NAME_SIZE;

    int rc = 0;


    ///step 3 send data using socket   
    while(1){
        rc = _send_pkt_out(udp_sock_fd, send_buffer, total_data_size, SRC_NODE_UDP_PORT_NO);
        n_pkts_send++;
        printf("No of byets send = %d, pkt no = %u\n",rc, n_pkts_send);
        //semd 10 pk per second
        usleep(100*1000);
    }
    close(udp_sock_fd);
    return 0;

}


















