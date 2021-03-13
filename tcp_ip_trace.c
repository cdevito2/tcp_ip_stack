/*
 * =====================================================================================
 *
 *       Filename:  tcp_ip_trace.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-03-13 11:17:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "tcp_public.h"

static char tcp_print_recv_buffer[TCP_PRINT_BUFFER_SIZE];
static char string_buffer[35];


static void init_string_buffer(){
    memset(string_buffer,0,sizeof(string_buffer));
}


static char * string_ethernet_hdr_type(unsigned short type){
    init_string_buffer();
    switch(type){
        case ETH_IP:
            strncpy(string_buffer,"ETH_IP",strlen("ETH_IP"));
            break;
        case ARP_MSG:
            strncpy(string_buffer,"ARP_MSG",strlen("ARP_MSG"));
        default:
            break;
    }
   return string_buffer;
}

static char * string_ip_hdr_protocol(uint8_t type){
    init_string_buffer();
    switch(type){
        case ICMP_PRO:
            strncpy(string_buffer, "ICMP_PRO", strlen("ICMP_PRO"));
            break;
        default:
            return NULL;
    }
    return string_buffer;
}

static char * string_arp_hdr_type(int type){
    init_string_buffer();
    switch(type){
        case ARP_BROAD_REQ:
            strncpy(string_buffer,"ARP_BROAD_REQ",strlen("ARP_BROAD_REQ"));
            break;
        case ARP_REPLY:
            strncpy(string_buffer, "ARP_REPLY",strlen("ARP_REPLY"));
            break;
        default:
            ; 
    }
}





static int tcp_dump_appln_hdr_protocol_icmp(char *buff, char *appln_data, uint32_t pkt_size){
    return 0;
    //my application icmp doesnt have a header currently
}


static int tcp_dump_ip_hdr(char *buff, ip_hdr_t *ip_hdr, uint32_t pkt_size){

    //convert the ip hdr ip addrs to printable format
    int rc = 0; //keep track of bytes written into buffer
    char ip1[16];
    char ip2[16];

    tcp_ip_convert_ip_n_to_p(ip_hdr->src_ip,ip1);
    tcp_ip_convert_ip_n_to_p(ip_hdr->dst_ip,ip2);

    //sprintf prints text to an array of char instead of a stream so we use to put in txt
    rc += sprintf(buff+rc, "IP Hdr :");
    rc += sprintf(buff+rc, "TL: %dB PRO: %s %s -> %s ttl :%d\n",
                    IP_HDR_TOTAL_LEN_IN_BYTES(ip_hdr), string_ip_hdr_protocol(ip_hdr->protocol),
                    ip1,ip2,ip_hdr->ttl);

    switch(ip_hdr->protocol){
        case ICMP_PRO:
            rc += tcp_dump_appln_hdr_protocol_icmp(buff+rc,INCREMENT_IPHDR(ip_hdr),IP_HDR_PAYLOAD_SIZE(ip_hdr));

            break;
        default:
            break;
            
    }
    return rc;

}


static int tcp_dump_arp_hdr(char *buff, arp_hdr_t *arp_hdr, uint32_t pkt_size){
    int rc =0;
    char ip1[16];
    char ip2[16];

    rc += sprintf(buff, "ARP Hdr : "); //TODO: might be buff + rc if its not working
    rc += sprintf(buff + rc, "Arp Type: %s %02x:%02x:%02x:%02x:%02x:%02x -> "
           "%02x:%02x:%02x:%02x:%02x:%02x %s -> %s\n",
           string_arp_hdr_type(arp_hdr->op_code),
           arp_hdr->src_mac.mac[0],
           arp_hdr->src_mac.mac[1],
           arp_hdr->src_mac.mac[2],
           arp_hdr->src_mac.mac[3],
           arp_hdr->src_mac.mac[4],
           arp_hdr->src_mac.mac[5],

           arp_hdr->dst_mac.mac[0],
           arp_hdr->dst_mac.mac[1],
           arp_hdr->dst_mac.mac[2],
           arp_hdr->dst_mac.mac[3],
           arp_hdr->dst_mac.mac[4],
           arp_hdr->dst_mac.mac[5],

           tcp_ip_convert_ip_n_to_p(arp_hdr->src_ip,ip1),
           tcp_ip_convert_ip_n_to_p(arp_hdr->dst_ip,ip2));
   return rc;

}



static int tcp_dump_ethernet_hdr(char *buff, ethernet_hdr_t *eth_hdr, uint32_t pkt_size){

    //have to check if its vlan tagged or not
    int rc=0;
    vlan_ethernet_hdr_t *vlan_eth_hdr = NULL;

    uint32_t payload_size = pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(eth_hdr);

    //check if tagged
    vlan_8021q_hdr_t *vlan_8021q_hdr = is_pkt_vlan_tagged(eth_hdr);

    if(vlan_8021q_hdr){
        vlan_eth_hdr = (vlan_ethernet_hdr_t *)eth_hdr;
    }

    unsigned short type = vlan_8021q_hdr ? vlan_eth_hdr->type : eth_hdr->type;

    //start the print statements

    rc += sprintf(buff + rc, "ETH Hdr : ");
    rc += sprintf(buff + rc, "%02x:%02x:%02x:%02x:%02x:%02x -> "
                        "%02x:%02x:%02x:%02x:%02x:%02x %-4s Vlan: %d PL: %dB\n",
           eth_hdr->src_mac.mac[0],
           eth_hdr->src_mac.mac[1],
           eth_hdr->src_mac.mac[2],
           eth_hdr->src_mac.mac[3],
           eth_hdr->src_mac.mac[4],
           eth_hdr->src_mac.mac[5],

           eth_hdr->dst_mac.mac[0],
           eth_hdr->dst_mac.mac[1],
           eth_hdr->dst_mac.mac[2],
           eth_hdr->dst_mac.mac[3],
           eth_hdr->dst_mac.mac[4],
           eth_hdr->dst_mac.mac[5],

           string_ethernet_hdr_type(type),

           vlan_8021q_hdr ? GET_8021Q_VLAN_ID(vlan_8021q_hdr) : 0,
           payload_size);

    //eth hdr can encapsulate ip hdr or arp hdr so check

    switch(type){
        case ETH_IP:
            rc += tcp_dump_ip_hdr(buff +rc, (ip_hdr_t *)GET_ETHERNET_HDR_PAYLOAD(eth_hdr),
                                                payload_size);
            break;
        case ARP_MSG:
            rc+= tcp_dump_arp_hdr(buff + rc, (arp_hdr_t *)GET_ETHERNET_HDR_PAYLOAD(eth_hdr),
                                                payload_size);
        default:
            break;
    }



    return rc;

}


static void tcp_write_data(int sock_fd, FILE *log_file1, FILE *log_file2, char *out_buff, uint32_t buff_size){
    int rc;
    assert(out_buff);

    if(log_file1){
        //write to this fil
        rc = fwrite(out_buff, sizeof(char), buff_size, log_file1);

        fflush(log_file1);//flush logfile content
    }
    if(log_file2){
        //write to this fil
        rc = fwrite(out_buff, sizeof(char), buff_size, log_file2);

        fflush(log_file2);//flush logfile content
    }

    if(sock_fd == -1){
        //dont have to write to console
        return;
    }

    //else we have to write to console
    write(sock_fd, out_buff, buff_size);

}















static void tcp_dump(int sock_fd, FILE *log_file1, FILE *log_file2, char *pkt, uint32_t pkt_size, hdr_type_t hdr_type, char *out_buff, uint32_t write_offset, uint32_t out_buff_size){
    //sock FD is the socket file descriptor 0 or -1, 0 is written to console, -1 otherwise
    //log_file 1 is the node level ptr to where pkt contents are to be written
    //log_file 2 is the interface level ptr to where pkt contensts to be written
    //pkt is pointer to pkt
    //pkt_size is pkt size
    //hdr_type is the starting hdr type of pkt
    //out_buff is the buffer in which the pkt content is written
    //write_offset is the starting position in output buffer to write pkt content
    //out_buff_size is the size of the output buffer in bytes


    int rc =0;

    //first check hdr type
    switch(hdr_type){
        case ETH_HDR:
            rc = tcp_dump_ethernet_hdr(out_buff + write_offset, (ethernet_hdr_t *)pkt,pkt_size);
            break;
        case IP_HDR:
            rc = tcp_dump_ip_hdr(out_buff + write_offset, (ip_hdr_t *)pkt, pkt_size);
            break;
        default:
            break;
    }

    if(!rc){
        return;
    }

    //at this point output buffer is filled
    //invoke function to write to output sources
    tcp_write_data(sock_fd, log_file1, log_file2, out_buff, write_offset + rc);

}


void tcp_dump_recv_logger(node_t *node, interface_t *intf, char *pkt, uint32_t pkt_size, hdr_type_t hdr_type){

    //step1 : eval condition whether user enabled logging flags through CLIs
    

    //step2 : init logging buffer
    

    //step3 : call tcp_dump

}
void tcp_dump_send_logger(node_t *node, interface_t *intf, char *pkt, uint32_t pkt_size, hdr_type_t hdr_type){

    //step1 : eval condition whether user enabled logging flags through CLIs
    

    //step2 : init logging buffer
    

    //step3 : call tcp_dump
}


static FILE * initialize_node_log_file(node_t *node){
    //creating the file name
    char file_name[32];
    memset(file_name,0,sizeof(file_name));
    sprintf(file_name,"logs/%s.txt",node->node_name);

    //create the pointer to file
    FILE *fptr = fopen(file_name,"w");

    if(!fptr){
        printf("Error : could not open log file %s, errno = %d\n",
            file_name,errno);
        return 0;
    }

    return fptr;


}

static FILE * initialize_interface_log_file(interface_t *intf){
    //creating file name - TODO remove slashes from interface names because
    //it causes errors for this

    char file_name[64];
    memset(file_name,0,sizeof(file_name));

    node_t *node = intf->att_node;
    sprintf(file_name, "logs/%s-%s.txt", node->node_name, intf->if_name);

    //pointer to file, allow writing
    FILE *fptr = fopen(file_name,"w");

    if(!fptr){
        printf("Error : could not open log file %s, errno = %d\n",
            file_name,errno);
        return 0;
    }

    return fptr;



}

void tcp_ip_init_node_log_info(node_t *node){
    log_t *log_info = &node->log_info;
    log_info->all = FALSE;
    log_info->recv = FALSE;
    log_info->send = FALSE;
    log_info->is_stdout = FALSE;
    log_info->l3_fwd = FALSE;
    log_info->log_file = initialize_node_log_file(node);
}

void tcp_ip_init_intf_log_info(interface_t *interface){
    log_t *log_info     = &interface->log_info;
    log_info->all       = FALSE;
    log_info->recv      = FALSE;
    log_info->send      = FALSE;
    log_info->is_stdout = FALSE;
    log_info->log_file  = initialize_interface_log_file(interface);

}




