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

typedef struct arp_hdr_{
    short hw_type; //1 for ethernet cable
    short proto_type;//0x8000 for ipv4
    char hw_addr_len;//6 for mac
    char proto_addr_len;//4 for ipv4
    short op_code;//Arp Req or reply
    mac_add_t src_mac;
    unsigned int src_ip;
    mac_add_t dst_mac;
    unsigned int dst_ip;

}arp_hdr_t;

typedef struct ethernet_hdr_{
    mac_add_t dst_mac;
    mac_add_t src_mac; //6 bytes
    unsigned short type;
    char payload[248]; //max 1500 bytes but chose smaller
    unsigned int FCS; //CRC field - frame check sequence
}ethernet_hdr_t;
#pragma pack(pop)


//MACRO DEFINED TO EVALUATE THE SIZE OF THE ETHERNET HEADER EXCLUDING THE PAYLOAD
//what this does is it gets the size of the entire ethernet header struct and subtracts the payload size and returns
#define ETH_HDR_SIZE_EXCL_PAYLOAD   \
    (sizeof(ethernet_hdr_t) - sizeof(((ethernet_hdr_t *)0)->payload))






//MACRO DEFINED TO RETURN THE FCS VALUE PRESENT IN THE FRAME
//what this does it it gets the location of the start of the payload, then adds it to the payload size to get the end
//at the end of the payload size is the 4 bytes of FCS so we cast to an unsigned int and return
#define ETH_FCS(eth_hdr_ptr,payload_size)   \
    (*(unsigned int *)(((char *)(((ethernet_hdr_t *)eth_hdr_ptr)->payload)+payload_size)))





 /*  VLAN SUPPORT STRUCTURES */

//4 byte vlan header
#pragma pack(push,1)
typedef struct vlan_8021q_hdr_{
    unsigned short tpid; // set to 0x8100
    short tci_pcp : 3;  //3 bits, not using
    short tci_dei : 1; //1 bit, not using
    short tci_vid : 12; //12 bits. vlan id
} vlan_8021q_hdr_t;


typedef struct vlan_ethernet_hdr_{
    mac_add_t dst_mac;
    mac_add_t src_mac;
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
    glthread_t arp_glue;
    bool_t is_sane;
    //for future implementation list of pending packets for arp res
    glthread_t arp_pending_list;
}arp_entry_t;
GLTHREAD_TO_STRUCT(arp_glue_to_arp_entry, arp_entry_t, arp_glue);





/*  This function takes in an ethernet frame and determines if it is tagged or not */
/*  Returns NULL if frame not tagges, pointer to vlan hdr if it is tagged */
static inline vlan_8021q_hdr_t *is_pkt_vlan_tagged(ethernet_hdr_t *ethernet_hdr){
    //13th and 14th byte in the vlan tagged and non vlan tagged eth hdr
    if(ethernet_hdr->type == VLAN_8021Q_PROTO){
        return (vlan_8021q_hdr_t *)&(ethernet_hdr->type); //return pointer to the vlan tagged hdr
    }
    else{
        return NULL; //untagged frame
    }
}





//This function returns the VLAN ID present in the vlan tagged ethernet frame
static inline unsigned int GET_8021Q_VLAN_ID(vlan_8021q_hdr_t *vlan_8021q_hdr){
    return (unsigned int)(vlan_8021q_hdr->tci_vid);
}







#define VLAN_ETH_HDR_SIZE_EXCL_PAYLOAD  \
    (sizeof(vlan_ethernet_hdr_t) - sizeof(((vlan_ethernet_hdr_t *)0)->payload))




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



/*  This function sets the FCS value of an untagged or a tagged eth frame */
static inline void SET_COMMON_ETH_FCS(ethernet_hdr_t *ethernet_hdr, unsigned int payload_size, unsigned int new_fcs){

    if(is_pkt_vlan_tagged(ethernet_hdr)){ //check if tagged
        VLAN_ETH_FCS(ethernet_hdr,payload_size) = new_fcs;
    }
    else{
        ETH_FCS(ethernet_hdr,payload_size) = new_fcs;
    }
}

static inline ethernet_hdr_t *
ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){

    char *temp = calloc(1, pkt_size);
    memcpy(temp, pkt, pkt_size);

    ethernet_hdr_t *eth_hdr = (ethernet_hdr_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset((char *)eth_hdr, 0, ETH_HDR_SIZE_EXCL_PAYLOAD);
    memcpy(eth_hdr->payload, temp, pkt_size);
    SET_COMMON_ETH_FCS(eth_hdr, pkt_size, 0);
    free(temp);
    return eth_hdr;
}



/*  

//static function to allocate ethernet header with payload packet
static inline ethernet_hdr_t * ALLOC_ETH_HDR_WITH_PAYLOAD(char *pkt, unsigned int pkt_size){
    //must encapsulate data into payload of ethernet header
    //initialize all fields of ethernet header to zero also
    
    char *temp = calloc(1,pkt_size);
    memcpy(temp,pkt,pkt_size);

    ethernet_hdr_t *header = (ethernet_hdr_t *)(pkt - ETH_HDR_SIZE_EXCL_PAYLOAD);
    memset((char *)header,0,ETH_HDR_SIZE_EXCL_PAYLOAD);
    //header is now pointing to where the payload data should be written to in the ethernet header
    memset(header,0,ETH_HDR_SIZE_EXCL_PAYLOAD);
    //above line sets all fields before the payload to be zero
    memcpy(header->payload,temp,pkt_size);
    free(temp);
    //set the FCS to be zero
    SET_COMMON_ETH_FCS(header,pkt_size, 0);    

}
*/

//static function to decide whether routing device should accept or reject incoming packet
static inline bool_t l2_frame_recv_qualify_on_interface(interface_t *interface, ethernet_hdr_t *ethernet_hdr, unsigned int *output_vlan_id){


    vlan_8021q_hdr_t *vlan_8021q_hdr = is_pkt_vlan_tagged(ethernet_hdr);
    
    //case where interface is not L3 or L2 mode, reject pkt
    if(!IS_INTF_L3_MODE(interface) && IF_L2_MODE(interface)==L2_MODE_UNKNOWN){
        return FALSE;
    }

    //case where interface in L3 mode and pkt is vlan tagged, drop pkt
    if(IS_INTF_L3_MODE(interface) && vlan_8021q_hdr){
        return FALSE;
    }

    //case where interface in L3 mode and dest mac is broadcast, accept it
    if(IS_INTF_L3_MODE(interface) && IS_MAC_BROADCAST_ADDR(ethernet_hdr->dst_mac.mac)){
        return TRUE;
    }

    //case where interface is L3 mode and interface mac == dest mac
    if(IS_INTF_L3_MODE(interface) && memcmp(IF_MAC(interface), ethernet_hdr->dst_mac.mac, sizeof(mac_add_t))){
        return TRUE;
    }


    //case where interface is L2 MODE + access but not in a VLAN
    if(IF_L2_MODE(interface) == ACCESS && get_access_intf_operating_vlan_id(interface) == 0){
        //case 3 - recv untagged frame, accept
        if(!vlan_8021q_hdr){
            return TRUE;
        }
        else{
            //recv tagged frame, reject - case 4
            return FALSE;
        }
    }


    *output_vlan_id = 0;
    unsigned int intf_vlan_id =0;
    unsigned int pkt_vlan_id =0;
    //case where interface is L2 MODE + access and in a vlan
    if(IF_L2_MODE(interface) == ACCESS){
        intf_vlan_id = get_access_intf_operating_vlan_id(interface);
        
        
        //case 3 - if pkt not tagged and switch not in vlan
        if(!intf_vlan_id && !vlan_8021q_hdr){
            return TRUE;
        }


        //case 6 - switch in vlan but pkt not tagged, accept
        if(intf_vlan_id && !vlan_8021q_hdr){
            *output_vlan_id = intf_vlan_id;//tag frame with this vlan id
            return TRUE;
        }
        
        
        
        
        //if pkt vlan id = intf vlan id accept frame, else reject - case 5
        pkt_vlan_id = GET_8021Q_VLAN_ID(vlan_8021q_hdr);
        if(pkt_vlan_id == intf_vlan_id){
            return TRUE;
        }
        else{
            return FALSE;
        }


    }
   
    //case 7 and 8 - switch in trunk mode but recv untagged frame
    if(IF_L2_MODE(interface) == TRUNK){
        if(!vlan_8021q_hdr){
            return FALSE;
        }
    }


    //case 9
    if(IF_L2_MODE(interface) == TRUNK && vlan_8021q_hdr){
        //accept if pkt vlan id is configured on interface, else drop pkt
        pkt_vlan_id = GET_8021Q_VLAN_ID(vlan_8021q_hdr);
        if(is_trunk_interface_vlan_enabled(interface, pkt_vlan_id)){
            return TRUE;
        }
        else{
            return FALSE;
        }
    }

    
    

}














/*  This function returns the pointer to the start of the payload for a tagged or untagged ethernet hdr frame */
static inline char * GET_ETHERNET_HDR_PAYLOAD(ethernet_hdr_t *ethernet_hdr){
    if(is_pkt_vlan_tagged(ethernet_hdr)){ //check if its tagged
        (vlan_ethernet_hdr_t *)ethernet_hdr;
        return ethernet_hdr->payload;
    }
    else{
        return ethernet_hdr->payload;
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
void node_set_intf_vlan_membership(node_t *node, char *intf_name, unsigned int vlan_id);





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
