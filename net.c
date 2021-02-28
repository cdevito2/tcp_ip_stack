/*
 * =====================================================================================
 *
 *       Filename:  net.c
 *
 *    Description:  networking functions
 *
 *        Version:  1.0
 *        Created:  20-12-01 08:09:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  chris devito
 *   Organization:  
 *
 * =====================================================================================
 */
#include <netinet/in.h>
#include "graph.h"
#include <stdio.h>
#include "utils.h"
#include <stdlib.h>
#include <memory.h>
/*Just some Random number generator*/
static unsigned int
hash_code(void *ptr, unsigned int size){
    unsigned int value=0, i =0;
    char *str = (char*)ptr;
    while(i < size)
    {
        value += *str;
        value*=97;
        str++;
        i++;
    }
    return value;
}


/*Heuristics, Assign a unique mac address to interface*/
void
interface_assign_mac_address(interface_t *interface){
    node_t *node = interface->att_node; 
    if(!node)
        return;
    /*  ISSUE HERE WITH MAC ADDRESS CHRIS */
    unsigned int hash_code_val = 0;
    hash_code_val = hash_code(node->node_name, NODE_NAME_SIZE);
    hash_code_val *= hash_code(interface->if_name, IF_NAME_SIZE);
    memset(IF_MAC(interface), 0, sizeof(IF_MAC(interface)));
    memcpy(IF_MAC(interface), (char *)&hash_code_val, sizeof(unsigned int));

}



bool_t node_set_loopback_address(node_t *node, char *ip_addr){
    node->node_nw_prop.is_lb_addr_config = TRUE;
    //assign ip

    strncpy(NODE_LO_ADDR(node),ip_addr,16);
    NODE_LO_ADDR(node)[15] = '\0';

    //add as direct route to the routing table
    rt_table_add_direct_route(NODE_RT_TABLE(node),ip_addr,32);
    return TRUE;
}

bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask){
    interface_t *intf = get_node_if_by_name(node,local_if);
    //now we have the interface struct for the node interface we want to apply ip 
    strncpy(IF_IP(intf),ip_addr,16);
    IF_IP(intf)[16]='\0';
    intf->intf_nw_props.mask = mask;
    intf->intf_nw_props.is_ipadd_config = TRUE;

    //add to the routing table
    rt_table_add_direct_route(NODE_RT_TABLE(node),ip_addr,mask);
    return TRUE;
}



//funciton to convert ip from int form into A.B.C.D/x string form
void convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){
    //call the inet_ntop function
    inet_ntop(AF_INET,&ip_addr,output_buffer,16);
    output_buffer[15] = '\0';
}

//function to convert ip from str(A.B.C.D/x) into decimal form
unsigned int convert_ip_from_str_to_int(char *str_ip_addr){
    unsigned int ip = 0;
    //use inet_pton and htonl to handle endianess
    inet_pton(AF_INET,str_ip_addr,&ip);
    ip = htonl(ip);
    return ip;
}



bool_t is_same_subnet(char *ip_addr, char mask, char *other_ip_addr){
    char intf_subnet[16];
    char subnet2[16];

    memset(intf_subnet,0,16);
    memset(subnet2,0,16);
    apply_mask(ip_addr,mask,intf_subnet);
    apply_mask(other_ip_addr,mask,subnet2);

    if(strncmp(intf_subnet,subnet2,16) == 0){
        return TRUE;
    }
    return FALSE;
}

bool_t is_interface_l3_bidirectional(interface_t *interface){
    //if L2 mode return false
    if(IF_L2_MODE(interface) == ACCESS || IF_L2_MODE(interface) == TRUNK){
        return FALSE;
    }
    //if no ip address return false
    if(!IS_INTF_L3_MODE(interface)){
        return FALSE;
    }

    //now get the other interface
    interface_t *other_interface = &interface->link->intf1 == interface ? \
            &interface->link->intf2 : &interface->link->intf1;
    
    if(!other_interface){
        return FALSE;
    }

    //check that the interfaces are up
    if(!IF_IS_UP(interface) || !IF_IS_UP(other_interface)){
        return FALSE;
    }

    //check that the other interface is also in L3 mode
    if(IF_L2_MODE(other_interface) == ACCESS || IF_L2_MODE(other_interface) == TRUNK){
        return FALSE;
    }

    if(!IS_INTF_L3_MODE(other_interface)){
        return FALSE;
    }

    //now make sure the 2 interfaces are in the same subnet
    if(!(is_same_subnet(IF_IP(interface),IF_MASK(interface), IF_IP(other_interface)) && is_same_subnet(IF_IP(other_interface),IF_MASK(other_interface),IF_IP(interface)))){
        return FALSE;
    }
    return TRUE;
}




void dump_interface_stats(interface_t *interface){
    
    printf("%s   ::  PktTx : %u, PktRx : %u",
        interface->if_name, interface->intf_nw_props.pkt_sent,
        interface->intf_nw_props.pkt_recv);
}


void dump_node_interface_stats(node_t *node){
    interface_t *intf;
    uint32_t i=0;
    for(;i<MAX_INTF_PER_NODE;i++){
        intf = node->intf[i];
        if(!intf){
            return;
        }
        dump_interface_stats(intf);
        printf("\n");
    }
}






void dump_node_nw_props(node_t *node){
    
    printf("\nNode Name = %s, udp_port_no = %u\n", node->node_name, node->udp_port_number);
    if(node->node_nw_prop.is_lb_addr_config){
        printf("\t lo addr: %s/32\n",NODE_LO_ADDR(node));
    }
}


void dump_intf_props(interface_t *interface){
    dump_interface(interface);

    printf("\t If Status : %s\n", IF_IS_UP(interface) ? "UP" : "DOWN");
    if(interface->intf_nw_props.is_ipadd_config){
        printf("\t IP ADDR = %s/%u",IF_IP(interface),interface->intf_nw_props.mask);

        printf("\t MAC : %u:%u:%u:%u:%u:%u\n", 
                IF_MAC(interface)[0], IF_MAC(interface)[1],
                IF_MAC(interface)[2], IF_MAC(interface)[3],
                IF_MAC(interface)[4], IF_MAC(interface)[5]);
    }


    else{
         printf("\t l2 mode = %s", intf_l2_mode_str(IF_L2_MODE(interface)));
         printf("\t vlan membership : ");
         int i = 0;
         for(; i < MAX_VLAN_MEMBERSHIP; i++){
            if(interface->intf_nw_props.vlans[i]){
                printf("%u  ", interface->intf_nw_props.vlans[i]);
            }
         }
         printf("\n");
    }
}


void dump_nw_graph(graph_t *graph){
    node_t *node;
    glthread_t *curr;
    interface_t *interface;
    printf("Topology Name = %s\n",graph->topology_name);
    unsigned int i;
    ITERATE_GLTHREAD_BEGIN(&graph->node_list,curr){
        printf("%u\n",node->udp_port_number);
        node = graph_glue_to_node(curr);
        dump_node_nw_props(node);
        for (i=0;i<MAX_INTF_PER_NODE;i++){
            interface = node->intf[i];
            if(!interface){
                break;
            }
            dump_intf_props(interface);
        }
    }ITERATE_GLTHREAD_END(&graph->node_list,curr);


}


interface_t *node_get_matching_subnet_interface(node_t *node, char *ip_addr){
    //loop through all interfaces on a node
    //get the subnet for each interface ip
    //apply the subnet value on the ip address given in argument
    //return if match found, NULL if no match found
    unsigned int i;
    interface_t *intf;//this will be what we return if a match
    char *intf_address = NULL;
    char mask;
    char intf_subnet[16];
    char subnet2[16];
    for (i=0;i<MAX_INTF_PER_NODE;i++){
        intf = node->intf[i];
        if(!intf) return NULL;
        if(intf->intf_nw_props.is_ipadd_config == FALSE){
            continue;
        }
        //get interface ip address
        intf_address = IF_IP(intf);
        //get mask
        mask = intf->intf_nw_props.mask;
        //create the subnet- create memory space
        memset(intf_subnet,0,16);
        memset(subnet2,0,16);
        //call apply mask function that i created
        apply_mask(intf_address,mask,intf_subnet);
        apply_mask(ip_addr,mask,subnet2);
        //compare results
        if(strncmp(intf_subnet,subnet2,16)==0){
            //return the interface
            return intf;
        }
        
    }
}

/*  function that performs right shift of data on a packet buffer and returns pointer to start of data in the right shifted packet buffer */
//TODO: FUNCTION MIGHT NOT BE FINISHED?
char * pkt_buffer_shift_right(char *pkt, unsigned int pkt_size, unsigned int total_buffer_size){
    //note that totalbuffersize is MAX_PACKET_BUFFER_SIZE - IF_NAME_SIZE
    char *temp = NULL;
    bool_t need_temp_memory = FALSE;
    if(pkt_size * 2 > total_buffer_size){
        need_temp_memory = TRUE;
    }
    
    if(need_temp_memory){
        temp = calloc(1, pkt_size);
        memcpy(temp, pkt, pkt_size);
        memset(pkt, 0, total_buffer_size);
        memcpy(pkt + (total_buffer_size - pkt_size), temp, pkt_size);
        free(temp);
        return pkt + (total_buffer_size - pkt_size);
    }
    
    memcpy(pkt + (total_buffer_size - pkt_size), pkt, pkt_size);
    memset(pkt, 0, pkt_size);
    return pkt + (total_buffer_size - pkt_size);
}


unsigned int ip_addr_p_to_n(char *ip_addr){
    unsigned int ip_addr_int;
    inet_pton(AF_INET,ip_addr,&ip_addr_int);
    ip_addr_int = htonl(ip_addr_int);
    return ip_addr_int;
}


void ip_addr_n_to_p(unsigned int ip_addr, char *ip_addr_str){
    //use inet_ntop function but first ensure its in network byte order
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET,&ip_addr,ip_addr_str,16);
}


//funciton which vlan membership for an interface passed as an argument
unsigned int get_access_intf_operating_vlan_id(interface_t *interface){
    //only called in access mode so should only have 1 VLAN
    if(IF_L2_MODE(interface) == TRUNK){
        return;
    }
    return interface->intf_nw_props.vlans[0];
}




//function checks if a switch in trunk mode has a match for the passed in vlan id
bool_t is_trunk_interface_vlan_enabled(interface_t *interface, unsigned int vlan_id){
    if(IF_L2_MODE(interface) == ACCESS){
        return;
    }
    unsigned int i=0;
    for(;i<MAX_VLAN_MEMBERSHIP;i++){
        if(interface->intf_nw_props.vlans[i] == vlan_id){
            return TRUE;
        }
    }
    return FALSE;
}















