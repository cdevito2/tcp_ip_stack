/*
 * =====================================================================================
 *
 *       Filename:  l2switch.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-01-23 01:09:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include "../graph.h"
#include "layer2.h"
#include "../gluethread/glthread.h"

typedef struct mac_table_entry_{
    mac_add_t mac;//key
    char oif_name[IF_NAME_SIZE];
    glthread_t mac_entry_glue; //for linked list insertion

}mac_table_entry_t;
GLTHREAD_TO_STRUCT(mac_entry_glue_to_mac_entry, mac_table_entry_t, mac_entry_glue);

typedef struct mac_table_{
    glthread_t mac_entries; //just a linked list of mac tableentries
}mac_table_t;

#define IS_MAC_TABLE_ENTRY_EQUAL(mac_entry_1, mac_entry_2)  \
    (strncmp(mac_entry_1->mac.mac,mac_entry_2->mac.mac, sizeof(mac_add_t)) == 0 && \
        strncmp(mac_entry_1->oif_name, mac_entry_2->oif_name, IF_NAME_SIZE) ==0)

/*  CRUD APIs for MAC TABLE */

//Create or Update
 
bool_t mac_table_entry_add(mac_table_t *mac_table, mac_table_entry_t *mac_table_entry){
    //check if the mac entry already exists and/or needs updating
    mac_table_entry_t *old_entry = mac_table_lookup(mac_table, mac_table_entry->mac.mac);
    //if they are equal do not update
    if(old_entry && IS_MAC_TABLE_ENTRY_EQUAL(old_entry, mac_table_entry)){
        return FALSE'
    }
    //if exists but not equal delete old entry
    if(old_entry){
        delete_mac_table_entry(mac_table, old_entry->mac.mac);
    }

    //add new entry
    //init the linked list node
    init_glthread(&mac_table_entry->mac_entry_glue);
    //add it to the mac table linked list
    glthread_add_next(&mac_table->mac_entries, &mac_table_entry->mac_entry_glue);
}

//Replace
mac_table_entry_t *mac_table_lookup(mac_table_t *mac_table, char *mac){
    //iterate through mac table linked list
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){
        //get the entry from the glthread
        mac_table_entry = mac_entry_glue_to_mac_entry(curr)
        //compare the mac address of the entry to the function argument
        if(strncmp(mac_table_entry->mac.mac,mac,sizeof(mac_add_t)) == 0){
            //found a match
            return mac_table_entry;
        }
    }ITERATE_GLTHREAD_END(&mac_table->mac_entries,curr);
    return NULL;

}

//Delete
void delete_mac_table_entry(mac_table_t *mac_table, char *mac){
    //find the entry in the mac table using lookup function
    mac_table_entry_t *mac_table_entry;
    mac_table_entry = mac_table_lookup(mac_table, mac);
    //if it didnt find the entry return 
    if(!mac_table_entry){
        return;
    }
    //we found a match so delete it
    //remove from the linked list of mac table
    remove_glthread(&mac_table_entry->mac_table_glue);
    //free the memory
    free(mac_table_entry);


}


//Initialize
void init_mac_table(mac_table_t **mac_table){
    //calloc mem space
    *mac_table = calloc(1, sizeof(mac_table_t));
    //init the linked list
    init_glthread(&((*mac_table)->mac_entries));
}


static void l2_switch_perform_mac_learning(node_t *node, char *src_mac, char *if_name){
    bool_t rc;
    //create mac table entry
    mac_table_entry_t *mac_table_entry = calloc(1,sizeof(mac_table_entry_t));
    //copy the src mac and if name into the entry
    memcpy(mac_table_entry->mac.mac,src_mac, sizeof(mac_add_t));
    strncpy(mac_table_entry->oif_name,if_name,IF_NAME_SIZE);

    //esc char for interface
    mac_table_entry->oif_name[IF_NAME_SIZE -1] = '\0';
    //add the entry into the table
    rc = mac_table_entry_add(NODE_MAC_TABLE(node), mac_table_entry);
    if(rc==FALSE){
        free(mac_table_entry);
    }
}



static void l2_switch_forward_frame(node_t *node, interface_t *recv_interface, 
                                    char *pkt, unsigned int pkt_size){
    //purpose is to find the correct outgoing interface
    //todo this we do a mac lookup with the key being the destination mac address
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    //check if dest mac is broadcast address
    if(IS_MAC_BROADCAST_ADR(ethernet_hdr->dst_mac.mac)){
        //flood out of all l2 interfaces except current one
        send_pkt_flood_l2_intf_only(node, recv_intf, pkt, pkt_size);
    }

    //now do mac table lookup
    mac_table_entry_t *mac_table_entry = mac_table_lookup(NODE_MAC_TABLE(node), ethernet_hdr->dst_mac.mac);

    //if no match found flood pkt 
    if(!mac_table_entry){
        send_plt_flood_l2_intf_only(node, recv_intf, pkt, pkt_size);
    }

    //if match found send packet out of specific interface
    
    char *oif_name = mac_table_entry->oif_name;
    interface_t *oif = get_node_if_by_name(node, oif_name);
    if(!oif){
        return;
    }
    //send pkt 
    send_pkt_out(pkt,pkt_size,oif);

}



void l2_switch_recv_frame(interface_t *interface, char *pkt, unsigned int pkt_size){
    //2 functions are to perform mac learning and to forward frame
    //step 1: extract the src and destination mac address
    node_t *node = interface->att_node;
    
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;

    char *dst_mac = (char *)ethernet_hdr->dst_mac.mac;
    char *src_mac = (char *)ethernet_hdr->src_mac.mac;

    //perform mac learning to add to the mac table
    l2_switch_perform_mac_learning(node,src_mac, interface->if_name);

    l2_switch_forward_frame(node, interface, pkt, pkt_size);
}






void dump_mac_table(mac_table_t *mac_table){

    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;
    
    //iterate through mac table linked list
    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){

        mac_table_entry = mac_entry_glue_to_mac_entry(curr);
        printf("\tMAC : %u:%u:%u:%u:%u:%u   | Intf : %s\n", 
            mac_table_entry->mac.mac[0], 
            mac_table_entry->mac.mac[1],
            mac_table_entry->mac.mac[2],
            mac_table_entry->mac.mac[3], 
            mac_table_entry->mac.mac[4],
            mac_table_entry->mac.mac[5],
            mac_table_entry->oif_name);
    } ITERATE_GLTHREAD_END(&mac_table->mac_entries, curr);
}





