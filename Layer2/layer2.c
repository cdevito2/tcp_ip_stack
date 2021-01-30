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
#include <sys/socket.h>
#include "layer2.h"
#include <arpa/inet.h>


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
    if(old_entry){
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
    //create memory for new entry
    arp_entry_t *arp_entry = calloc(1,sizeof(arp_entry_t));
    src_ip = htonl(arp_hdr->src_ip);
    //convert ip to string form
    inet_ntop(AF_INET,&src_ip, &arp_entry->ip_addr.ip_addr, 16);
    arp_entry->ip_addr.ip_addr[15] = '\0';

    //copy mac address into arp entry
    memcpy(arp_entry->mac_addr.mac,arp_hdr->src_mac.mac,sizeof(mac_add_t));
    //also need the receiving interface for arp entry
    strncpy(arp_entry->oif_name,iif->if_name, IF_NAME_SIZE);
    //now add the entry to the table
    bool_t rc = arp_table_entry_add(arp_table,arp_entry);

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
        memcpy(ethernet_hdr->src_mac.mac, IF_MAC(oif), sizeof(mac_add_t));
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
        memset(arp_hdr->dst_mac.mac,0,sizeof(mac_add_t));
        //dest ip is the ip address passed in as the argument
        inet_pton(AF_INET,ip_addr,&arp_hdr->dst_ip);
        arp_hdr->dst_ip = htonl(arp_hdr->dst_ip);
        //only thing left is the FCS field of ethernet hdr
        //use macro written to access FCS
        ETH_FCS(ethernet_hdr, sizeof(arp_hdr_t)) = 0;//we arent using it for anything so set to 0
        
        //final thing is to send the packet out of local interface
        send_pkt_out((char *)ethernet_hdr, ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size, oif);

        //free allocated memory 
        free(ethernet_hdr);
    }
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
    ETH_FCS(ethernet_hdr_reply,sizeof(arp_hdr_t)) = 0;

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
void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    //first header is ethernet header
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    //check things to see if packet can be processed
    if(l2_frame_recv_qualify_on_interface(interface, ethernet_hdr) == FALSE){
        printf("L2 Frame rejected");
        return;
    }
    printf("L2 Frame accepted\n");

    //deside what to do with packet based on type field
    if(IS_INTF_L3_MODE(interface)){
        switch(ethernet_hdr->type){
            case ARP_MSG:
            {
                //access to payload
                arp_hdr_t *arp_hdr = (arp_hdr_t *)(ethernet_hdr->payload);
                //check if its a broadcast message or an arp reply
                switch(arp_hdr->op_code){
                    case ARP_BROAD_REQ:{
                        process_arp_broadcast_request(node, interface, ethernet_hdr);
                        break;
                    }
                    case ARP_REPLY:{
                        process_arp_reply_msg(node, interface, ethernet_hdr);
                        break;
                    }
                }
            }
            break;
            default:
                //assume ip packet
                //promote to layer3
                promote_pkt_to_layer3(node, interface, pkt, pkt_size);
                break;
        }
    }
    else if(IF_L2_MODE(interface) == ACCESS || IF_L2_MODE(interface) == TRUNK){
        //l2 switch received
        l2_switch_recv_frame(interface, pkt, pkt_size);
    }
    else{
        //do nothing
        return;
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













void node_set_intf_vlan_membership(node_t *node, char *intf_name, unsigned int vlan_id){
    interface_t *interface = get_node_if_by_name(node, intf_name);
    interface_set_vlan(node,interface,vlan_id);
}











void interface_set_l2_mode(node_t *node, interface_t *interface, char *l2_mode_option){
    intf_l2_mode_t intf_l2_mode;
    //handle all cases of what the mode can be(access/trunk/unknown)
    if(strncmp(l2_mode_option, "access", strlen("access")) == 0){
        //set to access
        intf_l2_mode = ACCESS;
    }
    if(strncmp(l2_mode_option, "trunk", strlen("trunk")) == 0){
        //set to trunk
        intf_l2_mode = TRUNK;
    }

    //now handle cases where the mode is already set and want to overwrite
    if(IS_INTF_L3_MODE(interface)){
        //disable and set to L2 mode
        interface->intf_nw_props.is_ipadd_config = FALSE;
        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }
    //handle case whre the mode is unknown
    if(IF_L2_MODE(interface) == L2_MODE_UNKNOWN){
        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }
    //handle case where its access mode and we want to change to trunk
    if(IF_L2_MODE(interface) == ACCESS && intf_l2_mode == TRUNK){
        IF_L2_MODE(interface) = intf_l2_mode;
        return;
    }
    //handle case where its trunk mode and we want to change to access
    if(IF_L2_MODE(interface) == TRUNK && intf_l2_mode == ACCESS){
        IF_L2_MODE(interface) = intf_l2_mode;
        //note access mode cannot have 2 or more vlans so remove all
        unsigned int i =0;
        for(; i<MAX_VLAN_MEMBERSHIP; i++){
            interface->intf_nw_props.vlans[i]=0;
        }
        return;
    }

}



ethernet_hdr_t *tag_pkt_with_vlan_id(etherent_hdr_t *etherent_hdr, unsigned int total_pkt_size, int vlan_id, unsigned int *new_pkt_size){
    //check if its already tagged, if it is, update the vlan id

    vlan_8021q_hdr_t *vlan_8021q_hdr = is_pkt_vlan_tagged(ethernet_hdr);
    unsigned int payload_size =0;
    *new_pkt_size=0;

    if(vlan_8021q_hdr){
        //pkt is already tagged so replace the vlan id 
        payload_size = total_pkt_size - VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;
        vlan_8021q_hdr->tci_vid = short(vlan_id);
        //update checksum
        SET_COMMON_ETH_FCS(ethernet_hdr,payload_size,0);
        *new_pkt_size = total_pkt_size;
        return etherent_hdr;
    }
    
    
    
    //copy dst,src, type into temp mem
    ethernet_hdr_t ethernet_hdr_old;
    memcpy((char *)&ethernet_hdr_old, (char *)ethernet_hdr, ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(ethernet_hdr_old.FCS));

    total_pkt_size = ETH_HDR_SIZE_EXCL_PAYLOAD;
    //shift pkt pointer to the left by creating space size of vlan 8021q hdr
    
    vlan_ethernet_hdr_t *vlan_ethernet_hdr = (vlan_ethernet_hdr_t *)((char *)etherent_hdr - sizeof(vlan_8021q_hdr_t));
    //init pkt buffer with 0, except payload
    
    memset((char *)vlan_ethernet_hdr,0,VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(vlan_ethernet_hdr->FCS));

    //fill fields by copying from temp memory
    
    memcpy(vlan_ethernet_hdr->dst_mac.mac, ethernet_hdr_old->dst_mac.mac,sizeof(mac_add_t));
    memcpy(vlan_ethernet_hdr->dst_mac.mac, ethernet_hdr_old->dst_mac.mac,sizeof(mac_add_t));
     
    //update 8021q hdr
    vlan_ethernet_hdr->vlan_8021q_hdr.tpid = VLAN_8021Q_PROTO;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_pcp = 0;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_dei = 0;
    vlan_ethernet_hdr->vlan_8021q_hdr.tci_vid = (short)vlan_id;
    vlan_ethernet_hdr->type = ethernet_hdr_old.type;
    SET_COMMON_ETH_FCS((ethernet_hdr_t *)vlan_ethernet_hdr,payload_size,0);
    *new_pkt_size = total_pkt_size + sizeof(vlan_8021q_hdr_t);
    return (ethernet_hdr_t *)vlan_ethernet_hdr; 

    //free tmp memory
}





ethernet_hdr_t *untag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr, unsigned int total_pkt_size, unsigned int *new_pkt_size){
    //if already untagged, do nothing

    *new_pkt_size =0;
    vlan_8021_hdr_t *vlan_8021_hdr = is_pkt_vlan_tagged(ethernet_hdr);
    if(!vlan_8021q_hdr){
        *new_pkt_size = total_pkt_size;
        return ethernet_hdr;
    }

    //strip the vlan hdr
    vlan_ethernet_hdr_t vlan_ethernet_hdr_old;

    //copy vlan tagged hdr temp memory from start of buffer to beginning of payload
    memcpy((char *)vlan_ethernet_hdr_old, (char *)ethernet_hdr, VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD - sizeof(vlan_etherner_hdr_old.FCS));

    ethernet_hdr = (ethernet_hdr_t *)((char *)ethernet_hdr + sizeof(vlan_8021q_hdr_t));

    
    memcpy(ethernet_hdr->dst_mac.mac, vlan_ethernet_hdr_old.dst_mac.mac, sizeof(mac_add_t));
    memcpy(ethernet_hdr->src_mac.mac, vlan_ethernet_hdr_old.src_mac.mac, sizeof(mac_add_t));
    ethernet_hdr->type = vlan_ethernet_hdr_old.type;

    unsigned int payload_size = total_pkt_size - VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;
    SET_COMMON_ETH_FCS(ethernet_hdr,payload_size,0);
    *new_pkt_size = total_pkt_size - sizeof(vlan_8021q_hdr_t);
    return ethernet_hdr;
}











void node_set_intf_l2_mode(node_t *node, char *intf_name, intf_l2_mode_t intf_l2_mode){
    //set access or trunk mode for an interface on a node
    interface_t *interface = get_node_if_by_name(node, intf_name);
    interface_set_l2_mode(node, interface, intf_l2_mode_str(intf_l2_mode));
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
    }ITERATE_GLTHREAD_END(&arp_table->arp_entries,curr);
}



