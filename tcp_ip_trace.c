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

extern graph_t *topo;

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

void tcp_ip_show_log_status(node_t *node){
    int i=0;
    interface_t *intf;
    log_t *log_info = &node->log_info;

    printf("Log Status : Device : %s\n",node->node_name);
    printf("\tall     : %s\n", log_info->all ? "ON" : "OFF");
    printf("\trecv    : %s\n", log_info->recv ? "ON" : "OFF");
    printf("\tsend    : %s\n", log_info->send ? "ON" : "OFF");
    printf("\tstdout  : %s\n", log_info->is_stdout ? "ON" : "OFF");
    printf("\tl3_fwd  : %s\n", log_info->l3_fwd ? "ON" : "OFF");

    for( ; i < MAX_INTF_PER_NODE; i++){
        intf = node->intf[i];
        if(!intf) continue;

        log_info = &intf->log_info;
        printf("\tLog Status : %s(%s)\n", intf->if_name, IF_IS_UP(intf) ? "UP" : "DOWN");
        printf("\t\tall     : %s\n", log_info->all ? "ON" : "OFF");
        printf("\t\trecv    : %s\n", log_info->recv ? "ON" : "OFF");
        printf("\t\tsend    : %s\n", log_info->send ? "ON" : "OFF");
        printf("\t\tstdout  : %s\n", log_info->is_stdout ? "ON" : "OFF");
    }

}


void tcp_ip_set_all_log_info_params(log_t *log_info, bool_t status){

    log_info->all    = status;
    log_info->recv   = status;
    log_info->send   = status;
    log_info->l3_fwd = status;
}


int traceoptions_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable){
    node_t *node;
    char *node_name;
    char *if_name;
    uint32_t flags;
    interface_t *intf;
    int cmdcode = -1;

    char *flag_val;
    log_t *log_info;

    tlv_struct_t *tlv = NULL;
    int CMDCODE = EXTRACT_CMD_CODE(tlv_buf);

    TLV_LOOP_BEGIN(tlv_buf, tlv){
        if(strncmp(tlv->leaf_id,"node-name",strlen("node-name")) == 0){
            node_name = tlv->value;
        }
        else if(strncmp(tlv->leaf_id,"of-name",strlen("if-name")) == 0){
            if_name = tlv->value;
        }
        else if(strncmp(tlv->leaf_id,"node-name",strlen("node-name")) == 0){
            flag_val = tlv->value;
        }
        else{
            assert(0);//crash program
        }
    }TLV_LOOP_END;

    //check which command was inputted 
    switch(CMDCODE){
        case CMDCODE_DEBUG_GLOBAL_STDOUT:
            topo->gstdout = TRUE;
            break;
        case CMDCODE_DEBUG_LOGGING_PER_NODE:
        case CMDCODE_DEBUG_SHOW_LOG_STATUS:
            node = get_node_by_node_name(topo,node_name);
            log_info = &node->log_info;
        break;
        case CMDCODE_DEBUG_LOGGING_PER_INTF:
            node = get_node_by_node_name(topo,node_name);
            intf = get_node_if_by_name(node,if_name);
            if(!intf){
                printf("Error no interface found for %s on node &s\n",if_name,node_name);
                return -1;
            }
            log_info = &intf->log_info;
        break;
        default:
            ;
    }
    if(CMDCODE == CMDCODE_DEBUG_LOGGING_PER_NODE ||
           CMDCODE == CMDCODE_DEBUG_LOGGING_PER_INTF){

        if(strcmp(flag_val,"all") ==0){
            tcp_ip_set_all_log_info_params(log_info,TRUE);

        }
        else if(strcmp(flag_val,"no_all") ==0){
            tcp_ip_set_all_log_info_params(log_info,FALSE);
            if(CMDCODE == CMDCODE_DEBUG_LOGGING_PER_NODE){
                //disable interface logging
                int i=0;
                interface_t *intf;
                for(;i<MAX_INTF_PER_NODE;i++){
                    intf = node->intf[i];
                    if(!intf){
                        continue;
                    }
                    tcp_ip_set_all_log_info_params(&intf->log_info,FALSE);
                }
            }
        }
        //check the other flags
        else if(strcmp(flag_val, "recv") == 0){
            log_info->recv = TRUE;
        }
        else if(strcmp(flag_val, "no-recv") == 0){
            log_info->recv = FALSE;
        }
        else if(strcmp(flag_val, "send") == 0){
            log_info->send = TRUE;
        }
        else if(strcmp(flag_val, "no-send") == 0){
            log_info->send = FALSE;
        }
        else if(strcmp(flag_val, "stdout") == 0){
            log_info->is_stdout = TRUE;
        }
        else if(strcmp(flag_val, "no-stdout") == 0){
            log_info->is_stdout = FALSE;
        }
        else if(strcmp(flag_val, "l3-fwd") == 0){
            log_info->l3_fwd = TRUE;
        }
        else if(strcmp(flag_val, "no-l3-fwd") == 0){
            log_info->l3_fwd = FALSE;
        }
    }
    else if(CMDCODE == CMDCODE_DEBUG_SHOW_LOG_STATUS){
        tcp_ip_show_log_status(node);
    }
    return 0;

}

static void display_expected_flag(param_t *param, ser_buff_t *tlv_buff){
    printf(" : all | no-all\n");
    printf(" : recv | no-recv\n");
    printf(" : send | no-send\n");
    printf(" : stdout | no-stdout\n");
    printf(" : l3-fwd | no-l3-fwd\n");

}


int validate_flag_values(char *value){
    int k = 0;
    int len = strlen(value);

    if( (strncmp(value, "all",      k = strlen("all"))       ==   0   && k  == len)          || 
        (strncmp(value, "no-all",   k = strlen("no-all"))    ==   0   && k  == len)          ||
        (strncmp(value, "recv",     k = strlen("recv"))      ==   0   && k  == len)          ||
        (strncmp(value, "no-recv",  k = strlen("no-recv"))   ==   0   && k  == len)          ||
        (strncmp(value, "send",     k = strlen("send"))      ==   0   && k  == len)          ||
        (strncmp(value, "no-send",  k = strlen("no-send"))   ==   0   && k  == len)          ||
        (strncmp(value, "stdout",   k = strlen("stdout"))    ==   0   && k  == len)          ||
        (strncmp(value, "no-stdout",k = strlen("no-stdout")) ==   0   && k  == len)          ||
        (strncmp(value, "l3-fwd",   k = strlen("l3-fwd"))    ==   0   && k  == len)          ||
        (strncmp(value, "no-l3-fwd",k = strlen("no-l3-fwd")) ==   0   && k  == len)){
        return VALIDATION_SUCCESS;
    }
    return VALIDATION_FAILED;

}


static void tcp_ip_build_node_traceoptions_cli(param_t *node_name_param){
    {
        static param_t traceoptions;
        init_param(&traceoptions, CMD, "traceoptions", 0, 0, INVALID, 0, "traceoptions");
        libcli_register_param(node_name_param, &traceoptions);
        {
            static param_t flag;
            init_param(&flag, CMD, "flag", 0, 0, INVALID, 0, "flag");
            libcli_register_param(&traceoptions, &flag);
            libcli_register_display_callback(&flag, display_expected_flag);
            {
                static param_t flag_val;
                init_param(&flag_val, LEAF, 0, traceoptions_handler, validate_flag_values, STRING, "flag-val", 
                        "<all | no-all | recv | no-recv | send | no-send | stdout | no-stdout | l3-fwd | no-l3-fwd>");
                libcli_register_param(&flag, &flag_val);
                set_param_cmd_code(&flag_val, CMDCODE_DEBUG_LOGGING_PER_NODE);
            }
        }
    }
}



static void tcp_ip_build_intf_traceoptions_cli(param_t *intf_name_param){
    {
        static param_t traceoptions;
        init_param(&traceoptions, CMD, "traceoptions", 0, 0, INVALID, 0, "traceoptions");
        libcli_register_param(intf_name_param, &traceoptions);
        {
            static param_t flag;
            init_param(&flag, CMD, "flag", 0, 0, INVALID, 0, "flag");
            libcli_register_param(&traceoptions, &flag);
            libcli_register_display_callback(&flag, display_expected_flag);
            {
                static param_t flag_val;
                init_param(&flag_val, LEAF, 0, traceoptions_handler, validate_flag_values, STRING, "flag-val", 
                    "<all | no-all | recv | no-recv | send | no-send | stdout | no-stdout>");
                libcli_register_param(&flag, &flag_val);
                set_param_cmd_code(&flag_val, CMDCODE_DEBUG_LOGGING_PER_INTF);
            }
        }
    }

}


extern void tcp_ip_traceoptions_cli(param_t *node_name_param,param_t *intf_name_param){
    if(node_name_param){
        tcp_ip_build_node_traceoptions_cli(node_name_param);
    }
    if(intf_name_param){
        tcp_ip_build_intf_traceoptions_cli(intf_name_param);
    }
}







