/*
 * =====================================================================================
 *
 *       Filename:  layer2.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-12-10 07:49:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "graph.h"
#include <stdio.h>
#include "comm.h"
#include <stdlib.h>
#include <sys/socket.h>
#include "layer2.h"
#include <arpa/inet.h>

void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    //entry point into TCP/IP stack from the bottom
}

void init_arp_table( arp_table_t **arp_table){
    //calloc arp table
    *arp_table =  calloc(1,sizeof(arp_table_t));
    //init linked list which will hold pointer to arp table entries
    init_glthread(&((*arp_table)->arp_entries));
}




arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr){
    //lookup entry using ip as key 
    //loop through arp table 
    glthread *curr;
    arp_entry_t *arp_entry;
    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries,curr){
        //for each entry compare the key of entry to the ip passed as argument
        arp_entry = graph_glue_to_arp_entry(curr);
        if(strncmp(arp_entry->ip_addr.ip_addr,ip_addr,16) == 0){
            return arp_entry; 
        }
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries,curr);
    return NULL;
}

/* TODO: FIX THIS AND THE HELPER FUNCTION */
void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr){
    //loop through arp table and for each entry compare the key, if they match we must delete
    arp_entry_t *arp_entry = arp_table_lookup(arp_table, ip_addr);
    if(! arp_entry){
        return;
    }
    delete_arp_entry(arp_entry);
}

/*  
void delete_arp_entry(arp_entry_t *arp_entry){
    glthread_t *curr;
    arp_pending_entry_t *arp_pending_entry;
    remove_glthread(arp_entry->arp_glue);
    ITERATE_GLTHREAD_BEGIN(
}

*/


bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry){
    //ensure that you arent entering an existing entry
    arp_entry_t *old_entry = arp_table_lookup(arp_table, arp_entry->ip_addr.ip_addr);

    //compare if old arp entry is equal to the one we are trying to add
    if(old_entry && memcmp(old_entry,arp_entry, sizeof(arp_entry_t)) ==0){
        //no need to do anything in this case
        return FALSE;
    }
    //case 2 arp entry exists but needs updating
    if(arp_entry_old){
        delete_arp_table_entry(arp_table, arp_entry->ip_addr.ip_addr);

    }
    //insert the new arp table entry into table
    init_glthread(&arp_entry->arp_glue);
    glthread_add_next(&arp_table->arp_entries, &arp_entry->arp_glue);
    return TRUE;
}


void arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif){
    //only src ip and src mac are of interest
    unsigned int src_ip =0;
    assert(arp_hdr->op_code == ARP_REPLY);
    //create memory for new entry
    arp_entry_t *arp_entry = calloc(1,sizeof(arp_entry_t));
    src_ip = htonl(arp_hdr->src_ip);
    //convert ip to string form
    inet_ntop(AF_INET,&src_ip, &arp_entry->ip_addr.ip_addr, 16);
    arp_entry->ip_addr.ip_addr[15] = '\0';

    //copy mac address into arp entry
    memcpy(&arp_entry->mac_addr.mac,arp_hdr->src_mac,sizeof(mac_add_t));
    //also need the receiving interface for arp entry
    strncpy(arp_entry->oif_name,iif->if_name, IF_NAME_SIZE);
    //now add the entry to the table
    bool rc = arp_table_entry_add(arp_table,arp_entry);

    if(rc == FALSE){
        //free memory
        free(arp_entry);
    }

}

void send_arp_broadcast_request(node_t *node, interface_t *oif, char *ip_addr){
    //node sending the arp broadcast on the interface for the ip address of which arp resolution is being done
    //reserve memory for the eth hdr and the arp hdr
    unsigned int payload_size = sizeof(arp_hdr_t);
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)calloc(1,ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size);
    
    //if the outgoing interface is null, which is until i implement layer3, api must find the outgoing interface to sent msg
    if(!oif){
        oif = node_get_matching_subnet_interface(node,ip_addr);
        //if this interface returns NULL arp resolution is not possible
        if(!oif){
            printf("Error : %s : No eligible subnet for ARP resolution for Ip-address : %s\n",node->node_name,ip_addr);
            return;
        }
        //arp res possible 
        //Prepare ethernet header
        //dest mac is FFFFFFFF, src mac is outgoing interface mac, type field is 806
        layer2_fill_with_broadcast_mac(ethernet_hdr->dst_mac.mac);
        //putting in src mac addr
        memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t))
        ethernet_hdr->type = ARP_MSG; //sets type to 806-defined in tcpconst
        
        //fill fields of ARP header-payload of ethernet header is the arp packet
        arp_hdr_t *arp_hdr = (arp_hdr_t *)ethernet_hdr->payload;
        //first 4 fields are constants
        arp_hdr->hw_type = 1;
        arp_hdr->proto_type = 0x8000;
        arp_hdr->hw_addr_len = sizeof(mac_add_t);
        arp_hdr->proto_addr_len = 4;
        //arp broadcast so opcode is 1
        arp_hdr->op_code = ARP_BROAD_REQ;
        //next field is the src mac address
        memcpy(arp_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
        //next field is the src ip address
        //convert interface ip to integer form , assign it to arp header
        inet_pton(AF_INET,IF_IP(oif), &arp_hdr->src_ip);
        arp_hdr->src_ip = htonl(arp_hdr->src_ip);
        //dest mac is set to 0 because we dont know it
        memset(arp_hdr->dest_mac.mac,0,sizeof(mac_add_t));
        //dest ip is the ip address passed in as the argument
        inet_pton(AF_INET,ip_addr,&arp_hdr->dest_ip);
        arp_hdr->dest_ip = htonl(arp_hdr->dest_ip);
        //only thing left is the FCS field of ethernet hdr
        //use macro written to access FCS
        ETH_FCS(ethernet_hdr, sizeof(arp_hdr_t)) = 0;//we arent using it for anything so set to 0
        
        //final thing is to send the packet out of local interface
        send_pkt_out((char *)ethernet_hdr, ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size, oif);

        //free allocated memory 
        free(ethernet_hdr);
    }
}
//these static functions do not have header defined in the .h file because they are private and no other api will invoke them
static void process_arp_broadcast_request(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr){

}
static void send_arp_reply_msg(ethernet_hdr_t *ethernet_hdr_in, interface_t *oif){

}
static void process_arp_reply_msg(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr){
    //make entry into arp table
}

void dump_arp_table(arp_table_t *arp_table){
    glthread_t *curr;
    arp_entry_t *arp_entry;
    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries,curr){
        arp_entry = arp_glue_to_arp_entry(curr);
        printf("IP : %s, MAC : %u:%u:%u:%u:%u:%U, OIF = %s\n",
            arp_entry->ip_addr.ip_addr,
            arp_entry->mac_addr.mac[0],
            arp_entry->mac_addr.mac[1],
            arp_entry->mac_addr.mac[2],
            arp_entry->mac_addr.mac[3],
            arp_entry->mac_addr.mac[4],
            arp_entry->mac_addr.mac[5],
            arp_entry->oif_name);
    }
}



