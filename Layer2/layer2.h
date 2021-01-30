/*
 * =====================================================================================
 *
 *       Filename:  layer2.h
 *
 *    Description:  Defining ethernet header
 *
 *        Version:  1.0
 *        Created:  20-12-10 06:25:19 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: Chris Devito 
 *   Organization:  
 *
 * =====================================================================================
 */


/*  PRAGMA IS USED TO ENSURE COMPILER DOES NOT ADD ANY PADDING BYTES TO ANY OF THE DEFINED FIELDS */

#ifndef __LAYER2__
#define __LAYER2__

#include "../net.h"
#include "../gluethread/glthread.h"
#include <stdlib.h> //this is needed for calloc
#include "../tcpconst.h"

#pragma pack(push,1)
typedef struct ethernet_hdr_{
    mac_add_t src_mac; //6 bytes
    mac_add_t dst_mac; //6 bytes
    char payload[248]; //max 1500 bytes but chose smaller
    unsigned int FCS; //CRC field - frame check sequence
    unsigned short type; //2 bytes
}ethernet_hdr_t;



typedef struct arp_hdr_{
    short hw_type; //1 for ethernet cable
    short proto_type;//0x8000 for ipv4
    char hw_addr_len;//6 for mac
    char proto_addr_len;//4 for ipv4
    short op_code;//Arp Req or reply
    mac_add_t src_mac;
    mac_add_t dst_mac;
    unsigned int src_ip;
    unsigned int dst_ip;

}arp_hdr_t;

 /*  VLAN SUPPORT STRUCTURES */

//4 byte vlan header
 typedef struct vlan_8021q_hdr_{
    unsigned short tpid; // set to 0x8100
    short tci_pcp : 3;  //3 bits, not using
    short tci_dei : 1; //1 bit, not using
    short tci_vid : 12; //12 bits. vlan id
 }


typedef struct vlan_ethernet_hdr_{
    mac_add_t dst_mac;
    mac_add_t src_mac;i
    vlan_8021q_hdr_t vlan_8021q_hdr;
    unsigned short type;
    char payload[248];
    unsigned int FCS;
}vlan_ethernet_hdr_t;

#pragma pack(pop)

typedef struct arp_table_{
    glthread_t arp_entries;
}arp_table_t;


/* /
typedef struct arp_entry_ arp_entry_t;
*/
typedef struct apr_entry_{
    ip_add_t ip_addr;//key for table
    mac_add_t mac_addr;
    char oif_name[IF_NAME_SIZE];
    bool_t is_sane;
    glthread_t arp_glue;
    //for future implementation list of pending packets for arp res
    glthread_t arp_pending_list;
}arp_entry_t;
GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry, arp_entry_t, arp_glue);




//MACRO DEFINED TO EVALUATE THE SIZE OF THE ETHERNET HEADER EXCLUDING THE PAYLOAD
//what this does is it gets the size of the entire ethernet header struct and subtracts the payload size and returns
#define ETH_HDR_SIZE_EXCL_PAYLOAD   \
    (sizeof(ethernet_hdr_t) - sizeof(((ethernet_hdr_t *)0)->payload))




#define VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD  \
    (sizeof(vlan_ethernet_hdr_t) - sizeof(((vlan_etherent_hdr_t *)0)->payload))




//MACRO DEFINED TO RETURN THE FCS VALUE PRESENT IN THE FRAME
//what this does it it gets the location of the start of the payload, then adds it to the payload size to get the end
//at the end of the payload size is the 4 bytes of FCS so we cast to an unsigned int and return
#define ETH_FCS(eth_hdr_ptr,payload_size)   \
    (*(unsigned int *)(((char *)(((ethernet_hdr_t *)eth_hdr_ptr)->payload)+payload_size)))




#define VLAN_ETH_FCS(vlan_eth_hdr_ptr,payload_size) \
    (*(unsigned int *)(((char *)(((vlan_ethernet_hdr_t *)vlan_eth_hdr_ptr)->payload)+payload_size)))



#define ETH_COMMON_FCS(eth_hdr_ptr,payload_size)    \
    (is_pkt_vlan_tagged(eth_hdr_ptr) ? VLAN_ETH_FCS(eth_hdr_ptr,payload_size) : \
        ETH_FCS(eth_hdr_ptr,payload_size))


//static function to allocate ethernet header with payload packet
static ethernet_hdr_t * ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    //must encapsulate data into payload of ethernet header
    //initialize all fields of ethernet header to zero also
    
    char *temp = calloc(pkt,pkt_size);
    memcpy(temp,pkt,pkt_size);

    ethernet_hdr_t *header = (ethernet_hdr_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset((char *)header,0,ETH_HDR_SIZE_EXCL_PAYLOAD)
    //header is now pointing to where the payload data should be written to in the ethernet header
    memset(header,0,ETH_HDR_SIZE_EXCL_PAYLOAD);
    //above line sets all fields before the payload to be zero
    memcpy(header->payload,temp,pkt_size);
    free(temp);
    //set the FCS to be zero
    SET_COMMON_ETH_FCS(header,pkt_size) = 0;    

}


//static function to decide whether routing device should accept or reject incoming packet
static inline bool_t l2_frame_recv_qualify_on_interface(interface_t *interface, ethernet_hdr_t *ethernet_hdr){
    //check if interface IP address has ip address, if not return false straight away
    //if above passes, check if the interface mac address is equal to the ethernet header dest mac addr, if it is then return true
    //if the above fails, also check if the dest mac is the broadcast mac, and return true
    //return false otherwise

    //first check for ip addr
    if(!IS_INTF_L3_MODE(interface)){
        return FALSE;
    }
    //second check comparing dest mac address to interface mac address
    //memcmp compares binary byte buffers we we must use this, must compare size of ethernet header mac address bytes to ensure they are a match
    if(memcmp(IF_MAC(interface),ethernet_hdr->dst_mac.mac,sizeof(mac_add_t)) == 0){
        //if returns 0 they are the same
        return TRUE;
    }
    
    
    
    //third check see if dest mac is broadcast address
    if(IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
        return TRUE;
    }

    return FALSE;

}




/*  This function takes in an ethernet frame and determines if it is tagged or not */
/*  Returns NULL if frame not tagges, pointer to vlan hdr if it is tagged */
static inline vlan_8021q_hdr_t *is_pkt_vlan_tagged(ethernet_hdr_t *ethernet_hdr){
    //13th and 14th byte in the vlan tagged and non vlan tagged eth hdr
    if(ethernet_hdr->type == 0x8100){
        return (vlan_8021q_hdr_t *)&(ethernet_hdr->type); //return pointer to the vlan tagged hdr
    }
    else{
        return NULL; //untagged frame
    }
}






//This function returns the VLAN ID present in the vlan tagged ethernet frame
static inline unsigned int GET_8021Q_VLAN_ID(vlan_8021q_hdr_t *vlan_8021q_hdr){
    return (unsigned int)(vlan_8021q_hdr->tcp_vid);
}





/*  This function returns the pointer to the start of the payload for a tagged or untagged ethernet hdr frame */
static inline char * GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr_t *ethernet_hdr){
    if(is_pkt_vlan_tagged(ethernet_hdr){ //check if its tagged
        (vlan_ethernet_hdr_t *)ethernet_hdr;
        return ethernet_hdr->payload;
    }
    else{
        return ethernet_hdr->payload;
    }
}


/*  This function sets the FCS value of an untagged or a tagged eth frame */
static inline void SET_COMMON_ETH_FCS(ethernet_hdr_t *ethernet_hdr, unsigned int payload_size, unsigned int new_fcs){

    if(is_pkt_vlan_tagged(ethernet_hdr)){ //check if tagged
        VLAN_ETH_FCS(ethernet_hdr,payload_size) = new_fcs;
    }
    else{
        ETH_FCS(Ethernet_hdr,payload_size) = new_fcs;
    }
}





/*  This function returns the ethernet hdr size excluding payload for a tagged or untagged frame */
static inline unsigned int GET_ETH_HDR_SIZE_EXCL_PAYLOAD(ethernet_hdr_t *ethernet_hdr){
    if(is_pkt_vlan_tagged(ethernet_hdr)){
        return VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD;
    }
    else{
        return ETH_HDR_SIZE_EXCL_PAYLOAD;
    }
}


        


/*  functions to ta/untagg frame with ethernet vlan id */
ethernet_hdr_t *tag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr, unsigned int total_pkt_size, int vlan_id, unsigned int *new_pkt_size);
ethernet_hdr_t *untag_pkt_with_vlan_id(ethernet_hdr_t *ethernet_hdr, unsigned int total_pkt_size, unsigned int *new_pkt_size);




void node_set_intf_l2_mode(node_t *node, char *intf_name, intf_l2_mode_t intf_l2_mode);
void node_set_intf_vlan_membership(node_t *node, char *intf_name unsigned int vlan_id);





//function to call when node is created during topology creation
void init_arp_table(arp_table_t **arp_table);




//CRUD operations on arp table
bool_t arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry);//CREATE
arp_entry_t * arp_table_lookup(arp_table_t *arp_table, char *ip_addr);//REPLACE
void arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif);//UPDATE
void delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr);//DELETE




//print arp table
void dump_arp_table(arp_table_t *arp_table);

void delete_arp_entry(arp_entry_t *arp_entry);
#endif
