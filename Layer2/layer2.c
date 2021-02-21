/*
 * =====================================================================================
 *
 *       Filename:  layer2.c
 *
 *    Description:  This file implements all the Data link Layer functionality
 *
 *        Version:  1.0
 *        Created:  Friday 20 September 2019 05:15:51  IST
 *       Revision:  1.0
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#include "graph.h"
#include <stdio.h>
#include "layer2.h"
#include <stdlib.h>
#include <sys/socket.h>
#include "comm.h"
#include <arpa/inet.h> /*for inet_ntop & inet_pton*/


void init_arp_table( arp_table_t **arp_table){
    //calloc arp table
    *arp_table =  calloc(1,sizeof(arp_table_t));
    //init linked list which will hold pointer to arp table entries
    init_glthread(&((*arp_table)->arp_entries));
}





arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr){
    //lookup entry using ip as key 
    //loop through arp table 
    glthread_t *curr;
    arp_entry_t *arp_entry;
    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries,curr){
        //for each entry compare the key of entry to the ip passed as argument
        arp_entry = arp_glue_to_arp_entry(curr);
        if(strncmp(arp_entry->ip_addr.ip_addr,ip_addr,16) == 0){
            return arp_entry; 
        }
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries,curr);
    return NULL;
}


static void delete_arp_pending_entry(arp_pending_entry_t *arp_pending_entry){
    remove_glthread(&arp_pending_entry->arp_pending_entry_glue);
    free(arp_pending_entry);
}
void delete_arp_entry(arp_entry_t *arp_entry){
    //glthread_t *curr;
    //remove_glthread(arp_entry->arp_glue);
    //free(arp_entry);
    glthread_t *curr;
    arp_pending_entry_t *arp_pending_entry;
    remove_glthread(&arp_entry->arp_glue);

    ITERATE_GLTHREAD_BEGIN(&arp_entry->arp_pending_list, curr){

        arp_pending_entry = arp_pending_entry_glue_to_arp_pending_entry(curr);
        delete_arp_pending_entry(arp_pending_entry);
    } ITERATE_GLTHREAD_END(&arp_entry->arp_pending_list, curr);

    free(arp_entry);
}

void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr){
    //loop through arp table and for each entry compare the key, if they match we must delete
    arp_entry_t *arp_entry = arp_table_lookup(arp_table, ip_addr);
    if(! arp_entry){
        return;
    }
    delete_arp_entry(arp_entry);
}


void
send_arp_broadcast_request(node_t *node,
                           interface_t *oif,
                           char *ip_addr){

    /*Take memory which can accomodate Ethernet hdr + ARP hdr*/
    unsigned int payload_size = sizeof(arp_hdr_t);
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)calloc(1, 
                ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size);

    if(!oif){
        oif = node_get_matching_subnet_interface(node, ip_addr);
        if(!oif){
            printf("Error : %s : No eligible subnet for ARP resolution for Ip-address : %s",
                    node->node_name, ip_addr);
            return;
        }
        if(strncmp(IF_IP(oif), ip_addr, 16) == 0){
            printf("Error : %s : Attemp to resolve ARP for local Ip-address : %s",
                    node->node_name, ip_addr);
            return;
        }
    }
    /*STEP 1 : Prepare ethernet hdr*/
    layer2_fill_with_broadcast_mac(ethernet_hdr->dst_mac.mac);
    memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
    ethernet_hdr->type = ARP_MSG;

    /*Step 2 : Prepare ARP Broadcast Request Msg out of oif*/
    arp_hdr_t *arp_hdr = (arp_hdr_t *)(GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr));
    arp_hdr->hw_type = 1;
    arp_hdr->proto_type = 0x0800;
    arp_hdr->hw_addr_len = sizeof(mac_add_t);
    arp_hdr->proto_addr_len = 4;

    arp_hdr->op_code = ARP_BROAD_REQ;

    memcpy(arp_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));

    inet_pton(AF_INET, IF_IP(oif), &arp_hdr->src_ip);
    arp_hdr->src_ip = htonl(arp_hdr->src_ip);

    memset(arp_hdr->dst_mac.mac, 0,  sizeof(mac_add_t));

    inet_pton(AF_INET, ip_addr, &arp_hdr->dst_ip);
    arp_hdr->dst_ip = htonl(arp_hdr->dst_ip);

    SET_COMMON_ETH_FCS(ethernet_hdr, sizeof(arp_hdr_t), 0); /*Not used*/

    /*STEP 3 : Now dispatch the ARP Broadcast Request Packet out of interface*/
    send_pkt_out((char *)ethernet_hdr, ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size,
                    oif);

    free(ethernet_hdr);
}

static void send_arp_reply_msg(ethernet_hdr_t *ethernet_hdr_in, interface_t *oif){
    //we need access to the broadcast message to whcich we are replying 
    arp_hdr_t *arp_hdr_in = (arp_hdr_t *)(GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr_in));
    //need to create new packet- to encapsulate the arp in
    ethernet_hdr_t *ethernet_hdr_reply = (ethernet_hdr_t *)calloc(1, MAX_PACKET_BUFFER_SIZE);
    //move the src mac of the broadcast message to dest mac of the reply
    memcpy(ethernet_hdr_reply->dst_mac.mac, arp_hdr_in->src_mac.mac, sizeof(mac_add_t));
    //create the src mac address of the reply
    memcpy(ethernet_hdr_reply->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
    //set type to 806 
    ethernet_hdr_reply->type = ARP_MSG;
    //now prepare contents of arp reply msg
    arp_hdr_t *arp_hdr_reply = (arp_hdr_t *)(GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr_reply));
    //fill the 4 const fields
    arp_hdr_reply->hw_type = 1;
    arp_hdr_reply->proto_type = 0x8000;
    arp_hdr_reply->hw_addr_len = sizeof(mac_add_t);
    arp_hdr_reply->proto_addr_len = 4;
    //opcode is arp reply which is defined in tcpconst which is 2
    arp_hdr_reply->op_code = ARP_REPLY;
    //set arp reply src mac
    memcpy(arp_hdr_reply->src_mac.mac,IF_MAC(oif), sizeof(mac_add_t));
    // set src ip is ip of outgoing if
    inet_pton(AF_INET,IF_IP(oif), &arp_hdr_reply->src_ip);
    arp_hdr_reply->src_ip = htonl(arp_hdr_reply->src_ip);
    //set the dest mac to the src mac of the arp broad msg
    memcpy(arp_hdr_reply->dst_mac.mac, arp_hdr_reply->src_mac.mac, sizeof(mac_add_t));

    //set dest ip to src ip of arp broad
    arp_hdr_reply->dst_ip = arp_hdr_in->src_ip;
    //set FCS to 0
    SET_COMMON_ETH_FCS(ethernet_hdr_reply,sizeof(arp_hdr_t),0);

    unsigned int total_pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD + sizeof(arp_hdr_t);
    //right shift packet in the buffer

    char *shifted_pkt_buffer = pkt_buffer_shift_right((char *)ethernet_hdr_reply, total_pkt_size, MAX_PACKET_BUFFER_SIZE);
    //now send packet and free memory
    send_pkt_out(shifted_pkt_buffer, total_pkt_size, oif);
    free(ethernet_hdr_reply);

}

//these static functions do not have header defined in the .h file because they are private and no other api will invoke them
static void process_arp_broadcast_request(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr){
    //print that msg has been recv
    printf("%s: ARP broadcast msg recvd on interface %s of node %s\n", __FUNCTION__, iif->if_name, iif->att_node->node_name);

    //check if node is eligible to process msg
    //if its not, simply discard
    //the criteria to check is it the dest ip in arp broad matches the ip address on recp interface of recp node
    char ip_addr[16];
    //get the arp header
    arp_hdr_t *arp_hdr = (arp_hdr_t *)(GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr));
    // grab dest ip
    unsigned int arp_dst_ip = htonl(arp_hdr->dst_ip);
    inet_ntop(AF_INET,&arp_dst_ip, ip_addr, 16);
    ip_addr[15] = '\0';
    //compare
    if(strncmp(IF_IP(iif),ip_addr,16)){
        printf("%s : ARP Broadcast req msg dropped, DEST IP address %s did not match with interface ip : %s\n",node->node_name, ip_addr, IF_IP(iif));
        return;
    }
    //if they are the same we need to send an ARP reply message
    send_arp_reply_msg(ethernet_hdr, iif);
}





static void process_arp_reply_msg(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr){
    //make entry into arp table
    //invoked by node when node recv msg
    printf("%s : ARP reply msg recvd on interface %s of node %s\n", __FUNCTION__, iif->if_name, iif->att_node->node_name);
    //node has to make entry in arp table
    arp_table_update_from_arp_reply(NODE_ARP_TABLE(node),(arp_hdr_t *)GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr),iif);

}




extern void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt,
                                    unsigned int pkt_size, int L3_protocol_type);

extern void
l2_switch_recv_frame(interface_t *interface,
                     char *pkt, unsigned int pkt_size);




bool_t
arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry,
                    glthread_t **arp_pending_list){

    if(arp_pending_list){
        assert(*arp_pending_list == NULL);   
    }

    arp_entry_t *arp_entry_old = arp_table_lookup(arp_table, 
            arp_entry->ip_addr.ip_addr);

    /* Case 0 : if ARP table do not exist already, then add it
     * and return TRUE*/
    if(!arp_entry_old){
        glthread_add_next(&arp_table->arp_entries, &arp_entry->arp_glue);
        return TRUE;
    }
    

    /*Case 1 : If existing and new ARP entries are full and equal, then
     * do nothing*/
    if(arp_entry_old &&
            IS_ARP_ENTRIES_EQUAL(arp_entry_old, arp_entry)){

        return FALSE;
    }

    /*Case 2 : If there already exists full ARP table entry, then replace it*/
    if(arp_entry_old && !arp_entry_sane(arp_entry_old)){
        delete_arp_entry(arp_entry_old);
        init_glthread(&arp_entry->arp_glue);
        glthread_add_next(&arp_table->arp_entries, &arp_entry->arp_glue);
        return TRUE;
    }

    /*Case 3 : if existing ARP table entry is sane, and new one is also
     * sane, then move the pending arp list from new to old one and return FALSE*/
    if(arp_entry_old &&
        arp_entry_sane(arp_entry_old) &&
        arp_entry_sane(arp_entry)){
    
        if(!IS_GLTHREAD_LIST_EMPTY(&arp_entry->arp_pending_list)){
            glthread_add_next(&arp_entry_old->arp_pending_list,
                    arp_entry->arp_pending_list.right);
        }
        if(arp_pending_list)
            *arp_pending_list = &arp_entry_old->arp_pending_list;
        return FALSE;
    }

    /*Case 4 : If existing ARP table entry is sane, but new one if full,
     * then copy contents of new ARP entry to old one, return FALSE*/
    if(arp_entry_old && 
        arp_entry_sane(arp_entry_old) && 
        !arp_entry_sane(arp_entry)){

        strncpy(arp_entry_old->mac_addr.mac, arp_entry->mac_addr.mac, sizeof(mac_add_t));
        strncpy(arp_entry_old->oif_name, arp_entry->oif_name, IF_NAME_SIZE);
        arp_entry_old->oif_name[IF_NAME_SIZE -1] = '\0';

        if(arp_pending_list)
            *arp_pending_list = &arp_entry_old->arp_pending_list;
        return FALSE;
    }

    return FALSE;
}

arp_entry_t *
create_arp_sane_entry(arp_table_t *arp_table, char *ip_addr){ 

    /*case 1 : If full entry already exist - assert. The L2 must have
     * not create ARP sane entry if the already was already existing*/

    arp_entry_t *arp_entry = arp_table_lookup(arp_table, ip_addr);
    
    if(arp_entry){
        if(!arp_entry_sane(arp_entry)){
        /* Why are you creating ARP sane entry when ARP complete
         * entry is already present*/    
            assert(0);
        }
        return arp_entry;
    }

    /*if ARP entry do not exist, create a new sane entry*/
    arp_entry = calloc(1, sizeof(arp_entry_t));
    strncpy(arp_entry->ip_addr.ip_addr, ip_addr, 16);
    arp_entry->ip_addr.ip_addr[15] = '\0';
    init_glthread(&arp_entry->arp_pending_list);
    arp_entry->is_sane = TRUE;
    bool_t rc = arp_table_entry_add(arp_table, arp_entry, 0);
    if(rc == FALSE){
        assert(0);
    }
    return arp_entry;
}
static void 
pending_arp_processing_callback_function(node_t *node,
                                         interface_t *oif,
                                         arp_entry_t *arp_entry,
                                         arp_pending_entry_t *arp_pending_entry){

    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)arp_pending_entry->pkt;
    unsigned int pkt_size = arp_pending_entry->pkt_size;
    memcpy(ethernet_hdr->dst_mac.mac, arp_entry->mac_addr.mac, sizeof(mac_add_t));
    memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
    SET_COMMON_ETH_FCS(ethernet_hdr, pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr), 0);
    send_pkt_out((char *)ethernet_hdr, pkt_size, oif);
}


static void
process_arp_pending_entry(node_t *node, interface_t *oif, 
                          arp_entry_t *arp_entry, 
                          arp_pending_entry_t *arp_pending_entry){

    arp_pending_entry->cb(node, oif, arp_entry, arp_pending_entry);  
}


void
arp_table_update_from_arp_reply(arp_table_t *arp_table, 
                                arp_hdr_t *arp_hdr, interface_t *iif){

    unsigned int src_ip = 0;
    glthread_t *arp_pending_list = NULL;

    assert(arp_hdr->op_code == ARP_REPLY);

    arp_entry_t *arp_entry = calloc(1, sizeof(arp_entry_t));

    src_ip = htonl(arp_hdr->src_ip);

    inet_ntop(AF_INET, &src_ip, arp_entry->ip_addr.ip_addr, 16);

    arp_entry->ip_addr.ip_addr[15] = '\0';

    memcpy(arp_entry->mac_addr.mac, arp_hdr->src_mac.mac, sizeof(mac_add_t));

    strncpy(arp_entry->oif_name, iif->if_name, IF_NAME_SIZE);

    arp_entry->is_sane = FALSE;

    bool_t rc = arp_table_entry_add(arp_table, arp_entry, &arp_pending_list);

    glthread_t *curr;
    arp_pending_entry_t *arp_pending_entry;

    if(arp_pending_list){
        
        ITERATE_GLTHREAD_BEGIN(arp_pending_list, curr){
        
            arp_pending_entry = arp_pending_entry_glue_to_arp_pending_entry(curr);

            remove_glthread(&arp_pending_entry->arp_pending_entry_glue);

            process_arp_pending_entry(iif->att_node, iif, arp_entry, arp_pending_entry);
            
            delete_arp_pending_entry(arp_pending_entry);

        } ITERATE_GLTHREAD_END(arp_pending_list, curr);

        (arp_pending_list_to_arp_entry(arp_pending_list))->is_sane = FALSE;
    }

    if(rc == FALSE){
        delete_arp_entry(arp_entry);
    }
}


void
dump_arp_table(arp_table_t *arp_table){

    glthread_t *curr;
    arp_entry_t *arp_entry;

    ITERATE_GLTHREAD_BEGIN(&arp_table->arp_entries, curr){

        arp_entry = arp_glue_to_arp_entry(curr);
        printf("IP : %s, MAC : %u:%u:%u:%u:%u:%u, OIF = %s, Is Sane : %s\n", 
            arp_entry->ip_addr.ip_addr, 
            arp_entry->mac_addr.mac[0], 
            arp_entry->mac_addr.mac[1], 
            arp_entry->mac_addr.mac[2], 
            arp_entry->mac_addr.mac[3], 
            arp_entry->mac_addr.mac[4], 
            arp_entry->mac_addr.mac[5], 
            arp_entry->oif_name,
            arp_entry_sane(arp_entry) ? "TRUE" : "FALSE");
    } ITERATE_GLTHREAD_END(&arp_table->arp_entries, curr);
}

static void promote_pkt_to_layer2(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr, uint32_t pkt_size){

        switch(ethernet_hdr->type){
            case ARP_MSG:
            {
                //access to payload
                arp_hdr_t *arp_hdr = (arp_hdr_t *)(GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr));
                //check if its a broadcast message or an arp reply
                switch(arp_hdr->op_code){
                    case ARP_BROAD_REQ:{
                        process_arp_broadcast_request(node, iif, ethernet_hdr);
                        break;
                    }
                    case ARP_REPLY:{
                        process_arp_reply_msg(node, iif, ethernet_hdr);
                        break;
                    }
                }
            }
            break;
            case ETH_IP:
                promote_pkt_to_layer3(node,iif,GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr),
                        pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr),
                        ethernet_hdr->type);
                
            default:
                //assume ip packet
                //promote to layer3 - todo implement 
               // promote_pkt_to_layer3(node, interface, pkt, pkt_size);
                break;
        }
}
/*Interface config APIs for L2 mode configuration*/


void
interface_set_l2_mode(node_t *node, 
                      interface_t *interface, 
                      char *l2_mode_option){

    intf_l2_mode_t intf_l2_mode;

    if(strncmp(l2_mode_option, "access", strlen("access")) == 0){
        intf_l2_mode = ACCESS;    
    }
    else if(strncmp(l2_mode_option, "trunk", strlen("trunk")) ==0){
        intf_l2_mode = TRUNK;
    }
    else{
        assert(0);
    }

    /*Case 1 : if interface is working in L3 mode, i.e. IP address is configured.
     * then disable ip address, and set interface in L2 mode*/
    if(IS_INTF_L3_MODE(interface)){
        interface->intf_nw_props.is_ipadd_config = FALSE;

        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }

    /*Case 2 : if interface is working neither in L2 mode or L3 mode, then
     * apply L2 config*/
    if(IF_L2_MODE(interface) == L2_MODE_UNKNOWN){
        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }

    /*case 3 : if interface is operating in same mode, and user config same mode
     * again, then do nothing*/
    if(IF_L2_MODE(interface) == intf_l2_mode){
        return;
    }

    /*case 4 : if interface is operating in access mode, and user config trunk mode,
     * then overwrite*/
    if(IF_L2_MODE(interface) == ACCESS &&
            intf_l2_mode == TRUNK){
        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }

    /* case 5 : if interface is operating in trunk mode, and user config access mode,
     * then overwrite, remove all vlans from interface, user must enable vlan again 
     * on interface*/
    if(IF_L2_MODE(interface) == TRUNK &&
           intf_l2_mode == ACCESS){

        IF_L2_MODE(interface) = intf_l2_mode;

        unsigned int i = 0;

        for ( ; i < MAX_VLAN_MEMBERSHIP; i++){
            interface->intf_nw_props.vlans[i] = 0;
        }
    }
}


void interface_set_vlan(node_t *node, interface_t *interface, unsigned int vlan_id){
    //if interface is L3 configured cant set vlan
    if(IS_INTF_L3_MODE(interface)){
        printf("Error : Interface %s : L3 mode enabled \n",interface->if_name);
        return;
    }
    //if interface is unknown mode cant set vlan
    if(IF_L2_MODE(interface) != ACCESS && IF_L2_MODE(interface) != TRUNK){
        printf("Error : Interface %s : Unknown Mode \n", interface->if_name);
        return;
    }
    //if interface in ACCESS mode can only have 1 vlan
    if(IF_L2_MODE(interface) == ACCESS){
        //mabye change if statement to interface->intf_nw_props.intf_l2_mode == ACCESS
        unsigned int i=0;
        unsigned int *vlan = NULL;
        for ( ; i<MAX_VLAN_MEMBERSHIP;i++){
            if(interface->intf_nw_props.vlans[i]){
                vlan = &interface->intf_nw_props.vlans[i];
            }
        }
        if(vlan){
            *vlan = vlan_id;
            return;
        }
        interface->intf_nw_props.vlans[0] = vlan_id;
    }

    //add to list of vlans in trunk mode if its not at max
    if(IF_L2_MODE(interface) == TRUNK){
        unsigned int i=0;
        unsigned int *vlan = NULL;
        for(; i<MAX_VLAN_MEMBERSHIP;i++){
            if(!vlan && interface->intf_nw_props.vlans[i]==0){
                //add to list
                vlan = &interface->intf_nw_props.vlans[i];
            }
            else if(interface->intf_nw_props.vlans[i] == vlan_id){
                //vlan id is already configured for this interface
                return;
            }
        }
        if(vlan){
            //here is where we actually add to the list
            *vlan = vlan_id;
            return;
        }
        printf("Error : Interface %s : MAX Vlans limit", interface->if_name);
    }
}
/*APIs to be used to create topologies*/
void
node_set_intf_l2_mode(node_t *node, char *intf_name, 
                        intf_l2_mode_t intf_l2_mode){

    interface_t *interface = get_node_if_by_name(node, intf_name);
    assert(interface);

    interface_set_l2_mode(node, interface, intf_l2_mode_str(intf_l2_mode));
}

void
node_set_intf_vlan_membership(node_t *node, char *intf_name, 
                                unsigned int vlan_id){

    interface_t *interface = get_node_if_by_name(node, intf_name);
    assert(interface);

    interface_set_vlan(node, interface, vlan_id);
}

extern bool_t
is_layer3_local_delivery(node_t *node, 
                         uint32_t dst_ip);

static void
l2_forward_ip_packet(node_t *node, unsigned int next_hop_ip,
                    char *outgoing_intf, ethernet_hdr_t *pkt, 
                    unsigned int pkt_size){

    interface_t *oif = NULL;
    char next_hop_ip_str[16];
    arp_entry_t * arp_entry = NULL;
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    unsigned int ethernet_payload_size = pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD;

    next_hop_ip = htonl(next_hop_ip);
    inet_ntop(AF_INET, &next_hop_ip, next_hop_ip_str, 16);
    
    /*restore again, since htonl reverses the byte
     * order*/
    next_hop_ip = htonl(next_hop_ip);

    if(outgoing_intf) {

        /* Case 1 : Forwarding Case
         * It means, L3 has resolved the nexthop, So its 
         * time to L2 forward the pkt out of this interface*/
        oif = get_node_if_by_name(node, outgoing_intf);
        assert(oif);

        arp_entry = arp_table_lookup(NODE_ARP_TABLE(node), next_hop_ip_str);

        if (!arp_entry){

            /*Time for ARP resolution*/
            arp_entry = create_arp_sane_entry(NODE_ARP_TABLE(node),
                    next_hop_ip_str);
            
            add_arp_pending_entry(arp_entry,
                    pending_arp_processing_callback_function,
                    (char *)pkt, pkt_size);

            send_arp_broadcast_request(node, oif, next_hop_ip_str);
            return;

        }
        else if(arp_entry_sane(arp_entry)){
            add_arp_pending_entry(arp_entry,
                    pending_arp_processing_callback_function,
                    (char *)pkt, pkt_size);
            return;
        }
        else
            goto l2_frame_prepare ;
    }
   
    /*Case 4 : Self ping*/
    if(is_layer3_local_delivery(node, next_hop_ip)){

        promote_pkt_to_layer3(node, 0, GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr),
                pkt_size - GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr),
                ethernet_hdr->type);
        return;
    }

    /* case 2 : Direct host Delivery
       L2 has to forward the frame to machine on local connected subnet */
    oif = node_get_matching_subnet_interface(node, next_hop_ip_str);

    if(!oif){
        printf("%s : Error : Local matching subnet for IP : %s could not be found\n",
                    node->node_name, next_hop_ip_str);
        return;
    }

    arp_entry = arp_table_lookup(NODE_ARP_TABLE(node), next_hop_ip_str);

    if (!arp_entry){
        /*Time for ARP resolution*/
        arp_entry = create_arp_sane_entry(NODE_ARP_TABLE(node),
                next_hop_ip_str);

        add_arp_pending_entry(arp_entry,
                pending_arp_processing_callback_function,
                (char *)pkt, pkt_size);

        send_arp_broadcast_request(node, oif, next_hop_ip_str);
        return;
    }
    else if(arp_entry_sane(arp_entry)){
        add_arp_pending_entry(arp_entry,
                pending_arp_processing_callback_function,
                (char *)pkt, pkt_size);
        return;
    }
    l2_frame_prepare:
        memcpy(ethernet_hdr->dst_mac.mac, arp_entry->mac_addr.mac, sizeof(mac_add_t));
        memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
        SET_COMMON_ETH_FCS(ethernet_hdr, ethernet_payload_size, 0);
        send_pkt_out((char *)ethernet_hdr, pkt_size, oif);
}


static void layer2_pkt_receive_from_top(node_t *node, unsigned int next_hop_ip, char *outgoing_intf, char *pkt, unsigned int pkt_size, int protocol_number){
    
    //add assert here mabye in future
    //check protocol layer is ip protocol
    if(protocol_number = ETH_IP){
        //expand packet to make room for eth hdr
        ethernet_hdr_t *empty_ethernet_hdr = ALLOC_ETH_HDR_WITH_PAYLOAD(pkt,pkt_size);
        //init type field
        empty_ethernet_hdr->type = ETH_IP;

        l2_forward_ip_packet(node,next_hop_ip,outgoing_intf,empty_ethernet_hdr,pkt_size + ETH_HDR_SIZE_EXCL_PAYLOAD);

    }
}

void demote_pkt_to_layer2(node_t *node, unsigned int next_hop_ip, char *outgoing_intf, char *pkt, unsigned int pkt_size, int protocol_number){

    layer2_pkt_receive_from_top(node,next_hop_ip, outgoing_intf, pkt, pkt_size, protocol_number);
}



void
add_arp_pending_entry(arp_entry_t *arp_entry,
        arp_processing_fn cb,
        char *pkt,
        unsigned int pkt_size){

    arp_pending_entry_t *arp_pending_entry = 
        calloc(1, sizeof(arp_pending_entry_t) + pkt_size);

    init_glthread(&arp_pending_entry->arp_pending_entry_glue);
    arp_pending_entry->cb = cb;
    arp_pending_entry->pkt_size = pkt_size;
    memcpy(arp_pending_entry->pkt, pkt, pkt_size);

    glthread_add_next(&arp_entry->arp_pending_list, 
                    &arp_pending_entry->arp_pending_entry_glue);
}



/*Vlan Management Routines*/

/* Return new packet size if pkt is tagged with new vlan id*/

ethernet_hdr_t * 
tag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr, 
                     unsigned int total_pkt_size,
                     int vlan_id, 
                     unsigned int *new_pkt_size){

    unsigned int payload_size  = 0 ;
    *new_pkt_size = 0;

    /*If the pkt is already tagged, replace it*/
    vlan_8021q_hdr_t *vlan_8021q_hdr = 
        is_pkt_vlan_tagged(ethernet_hdr);

    
    if(vlan_8021q_hdr){
        payload_size = total_pkt_size - VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;
        vlan_8021q_hdr->tci_vid = (short)vlan_id;
        
        /*Update checksum, however not used*/
        SET_COMMON_ETH_FCS(ethernet_hdr, payload_size, 0);

        *new_pkt_size = total_pkt_size;
        return ethernet_hdr;
    }

    /*If the pkt is not already tagged, tag it*/
    /*Fix me : Avoid declaring local variables of type 
     ethernet_hdr_t or vlan_ethernet_hdr_t as the size of these
     variables are too large and is not healthy for program stack
     memory*/
    ethernet_hdr_t ethernet_hdr_old;
    memcpy((char *)&ethernet_hdr_old, (char *)ethernet_hdr, 
                ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(ethernet_hdr_old.FCS));

    payload_size = total_pkt_size - ETH_HDR_SIZE_EXCL_PAYLOAD; 
    vlan_ethernet_hdr_t *vlan_ethernet_hdr = 
            (vlan_ethernet_hdr_t *)((char *)ethernet_hdr - sizeof(vlan_8021q_hdr_t));

    memset((char *)vlan_ethernet_hdr, 0, 
                VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(vlan_ethernet_hdr->FCS));
    memcpy(vlan_ethernet_hdr->dst_mac.mac, ethernet_hdr_old.dst_mac.mac, sizeof(mac_add_t));
    memcpy(vlan_ethernet_hdr->src_mac.mac, ethernet_hdr_old.src_mac.mac, sizeof(mac_add_t));

    /*Come to 802.1Q vlan hdr*/
    vlan_ethernet_hdr->vlan_8021q_hdr.tpid = VLAN_8021Q_PROTO;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_pcp = 0;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_dei = 0;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_vid = (short)vlan_id;

    /*Type field*/
    vlan_ethernet_hdr->type = ethernet_hdr_old.type;

    /*No need to copy data*/

    /*Update checksum, however not used*/
    SET_COMMON_ETH_FCS((ethernet_hdr_t *)vlan_ethernet_hdr, payload_size, 0 );
    *new_pkt_size = total_pkt_size  + sizeof(vlan_8021q_hdr_t);
    return (ethernet_hdr_t *)vlan_ethernet_hdr;
}

/* Return new packet size if pkt is untagged with the existing
 * vlan 801.1q hdr*/
ethernet_hdr_t *
untag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr, 
                     unsigned int total_pkt_size,
                     unsigned int *new_pkt_size){

    *new_pkt_size = 0;

    vlan_8021q_hdr_t *vlan_8021q_hdr =
        is_pkt_vlan_tagged(ethernet_hdr);
    
    /*NOt tagged already, do nothing*/    
    if(!vlan_8021q_hdr){
        *new_pkt_size = total_pkt_size;
        return ethernet_hdr;
    }

    /*Fix me : Avoid declaring local variables of type 
     ethernet_hdr_t or vlan_ethernet_hdr_t as the size of these
     variables are too large and is not healthy for program stack
     memory*/
    vlan_ethernet_hdr_t vlan_ethernet_hdr_old;
    memcpy((char *)&vlan_ethernet_hdr_old, (char *)ethernet_hdr, 
                VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(vlan_ethernet_hdr_old.FCS));

    ethernet_hdr = (ethernet_hdr_t *)((char *)ethernet_hdr + sizeof(vlan_8021q_hdr_t));
   
    memcpy(ethernet_hdr->dst_mac.mac, vlan_ethernet_hdr_old.dst_mac.mac, sizeof(mac_add_t));
    memcpy(ethernet_hdr->src_mac.mac, vlan_ethernet_hdr_old.src_mac.mac, sizeof(mac_add_t));

    ethernet_hdr->type = vlan_ethernet_hdr_old.type;
    
    /*No need to copy data*/
    unsigned int payload_size = total_pkt_size - VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;

    /*Update checksum, however not used*/
    SET_COMMON_ETH_FCS(ethernet_hdr, payload_size, 0);
    
    *new_pkt_size = total_pkt_size - sizeof(vlan_8021q_hdr_t);
    return ethernet_hdr;
}




void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    //first header is ethernet header
    unsigned int vlan_id_to_tag =0;
    //check things to see if packet can be processed
     
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    if(l2_frame_recv_qualify_on_interface(interface, ethernet_hdr,&vlan_id_to_tag) == FALSE){
        printf("L2 Frame rejected");
        return;
    }
    
    
    printf("L2 Frame accepted on node %s\n",node->node_name);
    //pkt has passed all test cases in the qualify function

    //deside what to do with packet based on type field
    if(IS_INTF_L3_MODE(interface)){
        promote_pkt_to_layer2(node,interface,ethernet_hdr,pkt_size);
    }
    else if(IF_L2_MODE(interface) == ACCESS || IF_L2_MODE(interface) == TRUNK){

        //l2 switch received
        //check if frame needs to be tagged
        unsigned int new_pkt_size =0;
        if(vlan_id_to_tag){
            pkt = (char *)tag_pkt_with_vlan_id((ethernet_hdr_t *)pkt, pkt_size, vlan_id_to_tag, &new_pkt_size);

        }
        l2_switch_recv_frame(interface, pkt, vlan_id_to_tag ? new_pkt_size : pkt_size);
    }
    else{
        //do nothing
        return;
    }


}

