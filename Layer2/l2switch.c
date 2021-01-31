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
#include "comm.h"

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




//Initialize
void init_mac_table(mac_table_t **mac_table){
    //calloc mem space
    *mac_table = calloc(1, sizeof(mac_table_t));
    //init the linked list
    init_glthread(&((*mac_table)->mac_entries));
}






//Replace
mac_table_entry_t *mac_table_lookup(mac_table_t *mac_table, char *mac){
    //iterate through mac table linked list
    glthread_t *curr;
    mac_table_entry_t *mac_table_entry;

    ITERATE_GLTHREAD_BEGIN(&mac_table->mac_entries, curr){
        //get the entry from the glthread
        mac_table_entry = mac_entry_glue_to_mac_entry(curr);
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
    remove_glthread(&mac_table_entry->mac_entry_glue);
    //free the memory
    free(mac_table_entry);


}






//Create or Update
 
bool_t mac_table_entry_add(mac_table_t *mac_table, mac_table_entry_t *mac_table_entry){
    //check if the mac entry already exists and/or needs updating
    mac_table_entry_t *old_entry = mac_table_lookup(mac_table, mac_table_entry->mac.mac);
    //if they are equal do not update
    if(old_entry && IS_MAC_TABLE_ENTRY_EQUAL(old_entry, mac_table_entry)){
        return FALSE;
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






static bool_t l2_switch_send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *oif){
    //case 1 - if interface l3 mode assert
    assert(!IS_INTF_L3_MODE(oif));

    intf_l2_mode_t intf_l2_mode = IF_L2_MODE(oif);

    //case 6 unknown l2 mode
    if(intf_l2_mode == L2_MODE_UNKNOWN){
        return FALSE;
    }

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    vlan_8021q_hdr_t *vlan_8021q_hdr = is_pkt_vlan_tagged(ethernet_hdr);


    switch(intf_l2_mode){
        case ACCESS:
            {
                unsigned int intf_vlan_id = get_access_intf_operating_vlan_id(oif);
            
                //case 1 - not vlan enabled and pkt is not tagged, simply forward it
                if(!intf_vlan_id && !vlan_8021q_hdr){
                    send_pkt_out(pkt,pkt_size, oif);
                    return TRUE;
                }

                //case 2 - pkt is not tagged, interface vlan enabled, drop pkt
                if(intf_vlan_id && !vlan_8021q_hdr){
                    return FALSE;
                }

                //case 4 - pkt is tagged, interface not vlan enabled, drop pkt
                if(!intf_vlan_id && vlan_8021q_hdr){
                    return FALSE;
                }


                //case 5 - pkt is tagged and interface vlan enabled and the vlan id match
                if(vlan_8021q_hdr && (intf_vlan_id == GET_8021Q_VLAN_ID(vlan_8021q_hdr))){
                    //l2 switch untaggs frame here
                    unsigned int *new_pkt_size = 0;
                    ethernet_hdr = untag_pkt_with_vlan_id(ethernet_hdr, pkt_size, &new_pkt_size);
                    send_pkt_out((char *)ethernet_hdr, new_pkt_size, oif);
                    return TRUE;
                }
            }
            break;
        case TRUNK:
            {
                
               //pkt taggeed, int trunk mode, vlan match , l2 switch forward
               unsigned int pkt_vlan_id=0;
               if(vlan_8021q_hdr){
                   pkt_vlan_id = GET_8021Q_VLAN_ID(vlan_8021q_hdr);
               }

               if(pkt_vlan_id && is_trunk_interface_vlan_enabled(oif,pkt_vlan_id)){
                    send_pkt_out(pkt, pkt_size, oif);
                    return TRUE;
               }

               return FALSE;
            }
            break;
        case L2_MODE_UNKNOWN:
            break;
        default:
            ;
        return FALSE;

    }
}






//floods pkt out of all interfaces which satisfy conditions in the fcn l2 switch send pkt out
static bool_t l2_switch_flood_pkt_out(node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size){



    //need to create copy pkt so i can send it out of all interfaces
    interface_t *oif = NULL;
    unsigned int i=0;
    char *pkt_copy = NULL;
    char *temp_pkt = calloc(1, MAX_PACKET_BUFFER_SIZE);
    pkt_copy = temp_pkt + MAX_PACKET_BUFFER_SIZE - pkt_size;

    for(;i<MAX_INTF_PER_NODE;i++){
        oif = node->intf[i];
        if(!oif){
            break;
        }
        if(oif == exempted_intf || IS_INTF_L3_MODE(oif)){
            continue;
        }

        //else send pkt out of this node interface
        memcpy(pkt_copy,pkt,pkt_size);
        l2_switch_send_pkt_out(pkt_copy,pkt_size,oif);

    }
    free(temp_pkt);

}






static void l2_switch_forward_frame(node_t *node, interface_t *recv_interface, 
                                    char *pkt, unsigned int pkt_size){
    //purpose is to find the correct outgoing interface
    //todo this we do a mac lookup with the key being the destination mac address
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    //check if dest mac is broadcast address
    if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
        //flood out of all l2 interfaces except current one
        l2_switch_flood_pkt_out(node, recv_interface, (char *)ethernet_hdr, pkt_size);
    }

    //now do mac table lookup
    mac_table_entry_t *mac_table_entry = mac_table_lookup(NODE_MAC_TABLE(node), ethernet_hdr->dst_mac.mac);

    //if no match found flood pkt 
    if(!mac_table_entry){
        l2_switch_flood_pkt_out(node, recv_interface, (char *)ethernet_hdr, pkt_size);
    }

    //if match found send packet out of specific interface
    
    char *oif_name = mac_table_entry->oif_name;
    interface_t *oif = get_node_if_by_name(node, oif_name);
    if(!oif){
        return;
    }
    //send pkt 
    l2_switch_send_pkt_out((char *)ethernet_hdr,pkt_size,oif);

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





