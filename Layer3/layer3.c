/*
 * =====================================================================================
 *
 *       Filename:  layer3.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-07 12:46:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <stdio.h>
#include "graph.h"
#include "../Layer2/layer2.h"
#include "layer3.h"
#include <sys/socket.h>
#include <memory.h>
#include <stdlib.h>
#include "tcpconst.h"
#include "comm.h"
#include <arpa/inet.h> /*for inet_ntop & inet_pton*/

static bool_t l3_is_direct_route(l3_route_t *l3_route){
    return (l3_route->is_direct);
}


bool_t is_layer3_local_delivery(node_t *node, unsigned int dst_ip){
    //check if dest ip matches with interfaces of router
    char dest_ip_str[16];
    dest_ip_str[15] = '\0';
    char *intf_addr = NULL;
    dst_ip = htonl(dst_ip);
    inet_ntop(AF_INET,&dst_ip,dest_ip_str,16);


    //check if the dest ip is loopback
    if(strncmp(NODE_LO_ADDR(node),dest_ip_str,16)==0){
        return TRUE;
    }


    //else loop and check node interfaces
    uint32_t i=0;
    interface_t *intf;
    for (;i<MAX_INTF_PER_NODE;i++){
        intf=node->intf[i];
        if(!intf){
            return FALSE;
        }
        if(intf->intf_nw_props.is_ipadd_config == FALSE)
            continue;

        intf_addr = IF_IP(intf);
        if(strncmp(intf_addr,dest_ip_str,16)==0){
            return TRUE;
        }
    }
    return FALSE;
}





void init_rt_table(rt_table_t **rt_table){
    //calloc mem
    *rt_table = calloc(1,sizeof(rt_table_t));
    //init glthread
    init_glthread(&((*rt_table)->route_list));
}







l3_route_t * rt_table_lookup(rt_table_t *rt_table, char *ip_addr, char mask){
    glthread_t *curr;
    l3_route_t *l3_route;


    //loop through routing table
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list, curr){
        l3_route = rt_glue_to_l3_route(curr);
        if(strncmp(l3_route->dest,ip_addr,16) == 0 && l3_route->mask == mask){
            return l3_route;
        }
    } ITERATE_GLTHREAD_END(&rt_table->route_list,curr);
}





nexthop_t *l3_route_get_active_nexthop(l3_route_t *l3_route){
    if(l3_is_direct_route(l3_route)){
        return NULL;
    }

    nexthop_t *nexthop = l3_route->nexthops[l3_route->nxthop_idx];
    l3_route->nxthop_idx++;

    if(l3_route->nxthop_idx == MAX_NXT_HOPS || !l3_route->nexthops[l3_route->nxthop_idx]){
        l3_route->nxthop_idx = 0;
    }
    return nexthop;
}



void delete_rt_table_entry(rt_table_t *rt_table, char *ip_addr, char mask){
    char dst_str_with_mask[16];
    apply_mask(ip_addr,mask,dst_str_with_mask);
    l3_route_t *l3_route = rt_table_lookup(rt_table,dst_str_with_mask,mask);
    if(!l3_route){
        return;
    }

    //found route and delete it
    remove_glthread(&l3_route->rt_glue);
    free(l3_route);
}














static bool_t _rt_table_entry_add(rt_table_t *rt_table, l3_route_t *l3_route){
    //first check if route already exists
    l3_route_t *l3_route_old = rt_table_lookup(rt_table,l3_route->dest,l3_route->mask);



    init_glthread(&l3_route->rt_glue);
    glthread_add_next(&rt_table->route_list, &l3_route->rt_glue);
    return TRUE;


}



void rt_table_add_direct_route(rt_table_t *rt_table, char *dst, char mask){
    //no oif or gateway IP so call add route fcn with 0 passed for those
    rt_table_add_route(rt_table,dst,mask,0,0,0);
}


l3_route_t *
l3rib_lookup(rt_table_t *rt_table, 
             uint32_t dest_ip, 
             char mask){

    char dest_ip_str[16];
    glthread_t *curr = NULL;
    char dst_str_with_mask[16];
    l3_route_t *l3_route = NULL;

    tcp_ip_convert_ip_n_to_p(dest_ip, dest_ip_str);

    apply_mask(dest_ip_str, mask, dst_str_with_mask);

    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list, curr){

        l3_route = rt_glue_to_l3_route(curr);
        
        if(strncmp(dst_str_with_mask, l3_route->dest, 16) == 0 &&
            l3_route->mask == mask){
            return l3_route;
        }
    } ITERATE_GLTHREAD_END(&rt_table->route_list, curr);

    return NULL;
}



l3_route_t *l3rib_lookup_lpm(rt_table_t *rt_table,unsigned int dest_ip){
    l3_route_t *l3_route = NULL;
    l3_route_t *lpm_l3_route = NULL;
    l3_route_t *default_l3_rt = NULL;

    glthread_t *curr = NULL;
    char subnet[16];
    char dest_ip_str[16];
    char longest_mask = 0;

    dest_ip = htonl(dest_ip);
    inet_ntop(AF_INET,&dest_ip,dest_ip_str,16);
    dest_ip_str[15] = '\0';

    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,curr){
        l3_route = rt_glue_to_l3_route(curr);
        memset(subnet,0,16);
        apply_mask(dest_ip_str,l3_route->mask,subnet);

        if(strncmp("0.0.0.0",l3_route->dest,16) == 0 && l3_route->mask ==0){
            default_l3_rt = l3_route;
        }

        else if(strncmp(subnet,l3_route->dest,strlen(subnet)) == 0){
            if (l3_route->mask > longest_mask){
                longest_mask = l3_route->mask;
                lpm_l3_route = l3_route;
            }
        }
    }ITERATE_GLTHREAD_END(&rt_Table->route_list,curr);
    return lpm_l3_route ? lpm_l3_route : default_l3_rt;
}






void rt_table_add_route(rt_table_t *rt_table, char *dst, char mask, char *gw, interface_t *oif,uint32_t spf_metric){
    
    
    
    unsigned int dst_int;
    char dest_str_with_mask[16];
    bool_t new_route = FALSE;
    apply_mask(dst,mask,dest_str_with_mask);
    inet_pton(AF_INET,dest_str_with_mask,&dst_int);

    //dest route is the subnet ID which is why we apply mask
    l3_route_t *l3_route = l3rib_lookup(rt_table,dst_int,mask);

    if(!l3_route){
    //init mem
        l3_route = calloc(1,sizeof(l3_route_t));

    //copy the subnet ID into the dest field of the route
        strncpy(l3_route->dest,dest_str_with_mask,16);
    //esc char
        l3_route->dest[15] = '\0';

    //set mask field for route
        l3_route->mask = mask;
        new_route = TRUE;
        l3_route->is_direct = TRUE;


    }

    int i=0;
    if(!new_route){
        //get index into nexthop array - checking if route already exists
        for(;i<MAX_NXT_HOPS;i++){
            if(l3_route->nexthops[i]){
                if(strncmp(l3_route->nexthops[i]->gw_ip,gw,16) == 0 &&
                    l3_route->nexthops[i]->oif == oif){
                    printf("Error : Attempt to add duplicate route\n");
                    return;
                }
            }
        }
    }

    if(i == MAX_NXT_HOPS){
        printf("Error : no nexthops space left for route %s/%u\n",dest_str_with_mask,mask);
        return;
    }

    if(gw && oif){
        nexthop_t *nexthop = calloc(1,sizeof(nexthop_t));
        l3_route->nexthops[i] = nexthop;
        l3_route->is_direct = FALSE;
        l3_route->spf_metric = spf_metric;
        nexthop->ref_count++;
        //copy gateway ip field into l3 route
        strncpy(nexthop->gw_ip,gw,16);
        //esc char
        nexthop->gw_ip[15] = '\0';
        nexthop->oif = oif;
    }

    if(new_route){
    //add route into table
        if(! _rt_table_entry_add(rt_table,l3_route)){
            printf("Error : Route %s/%d Instantiation Failed \n",
                dest_str_with_mask,mask);
            free(l3_route);
        }

    }


}


extern void demote_pkt_to_layer2(node_t *node,unsigned int next_hop_ip, char *outgoing_intf, char *pkt, unsigned int pkt_size, int protocol_number);


static void layer3_pkt_receive_from_top(node_t *node, char *pkt, unsigned int size, int protocol_number,unsigned int dest_ip_address){

    ip_hdr_t ip_hdr;
    initialize_ip_hdr(&ip_hdr);

    //fill ip hdr fields
    ip_hdr.protocol = protocol_number;
    unsigned int addr_int = 0;
    inet_pton(AF_INET,NODE_LO_ADDR(node),&addr_int);
    addr_int = htonl(addr_int);

    ip_hdr.src_ip = addr_int;
    ip_hdr.dst_ip = dest_ip_address;


    //in vid he said to put as unsigned short?
    ip_hdr.total_length = (unsigned short)ip_hdr.ihl * 4 + (unsigned short)(size/4) + (unsigned short)((size %4) ? 1:0);

    //else we found a route, is it direct or not
    //first prepare a packet
    char *new_pkt = NULL;
    unsigned int new_pkt_size = 0;
    new_pkt_size = ip_hdr.total_length * 4;
    new_pkt = calloc(1,MAX_PACKET_BUFFER_SIZE);
    memcpy(new_pkt,(char *)&ip_hdr,ip_hdr.ihl*4);

    if(pkt && size){
        //ip hdr followed by application data
        memcpy(new_pkt+(ip_hdr.ihl*4),pkt,size);
    }


    l3_route_t *l3_route = l3rib_lookup_lpm(NODE_RT_TABLE(node), ip_hdr.dst_ip);

    if(!l3_route){
        printf("Node : %s : No L3 Route\n",node->node_name);
        free(new_pkt);
        return;
    }



    //is direct route?
    bool_t is_direct_route = l3_is_direct_route(l3_route);

    //create space so link layer can add ethernet hdr
    char *shifted_pkt_buffer = pkt_buffer_shift_right(new_pkt,new_pkt_size, MAX_PACKET_BUFFER_SIZE);
    if(is_direct_route){
        demote_pkt_to_layer2(node,dest_ip_address,0,shifted_pkt_buffer,new_pkt_size,ETH_IP);
        return;
    }

    //not direct need to send out of nexthop
    uint32_t next_hop_ip;
    nexthop_t *nexthop = NULL;

    nexthop = l3_route_get_active_nexthop(l3_route);

    if(!nexthop){
        free(new_pkt);
        return;
    }

    inet_pton(AF_INET,nexthop->gw_ip,&next_hop_ip);
    next_hop_ip = htonl(next_hop_ip);

    tcp_dump_l3_fwding_logger(node,nexthop->oif->if_name,nexthop->gw_ip);
    demote_pkt_to_layer2(node,next_hop_ip,nexthop->oif->if_name,shifted_pkt_buffer,new_pkt_size,ETH_IP);

    free(new_pkt);
}






void demote_packet_to_layer3(node_t *node, char *pkt, unsigned int size, int protocol_number, unsigned int dest_ip_address){

    layer3_pkt_receive_from_top(node,pkt,size,protocol_number,dest_ip_address);

}













static void layer3_ip_pkt_recv_from_layer2(node_t *node, interface_t *interface, ip_hdr_t *pkt, unsigned int pkt_size){
    

    char *l4_hdr;
    char *l5_hdr;
    //step 1 : lookup routing table for matching entry
    char dest_ip_addr[16];
    dest_ip_addr[15]='\0';
    ip_hdr_t *ip_hdr = pkt;
    unsigned int dest_ip = htonl(ip_hdr->dst_ip);
    inet_ntop(AF_INET,&dest_ip,dest_ip_addr,16);

    l3_route_t *l3_route = l3rib_lookup_lpm(NODE_RT_TABLE(node), ip_hdr->dst_ip);

    if(!l3_route){
        printf("Router %s : Cannot Route IP : %s\n",node->node_name,dest_ip_addr);
        return;
    }

    //found matching route
    //check if its a direct route
    if(l3_is_direct_route(l3_route)){
        
        
        //case 1 - local delivery to interface on this router
        if(is_layer3_local_delivery(node,ip_hdr->dst_ip)){
            l4_hdr = (char *)INCREMENT_IPHDR(ip_hdr);
            l5_hdr = l4_hdr;

            switch(ip_hdr->protocol){
                case ICMP_PRO:
                    printf("IP Address : %s, ping success \n",dest_ip_addr);
                    break;
                default:
                    ; 
            }
            return;
        }


        //case 2 - pkt for a machine in local subnet
        //demote to link layer for arp and/or l2 forwarding
        demote_pkt_to_layer2(node,0,NULL,(char *)ip_hdr, pkt_size, ETH_IP);
        return;

    }
    else{
        //its a remote subnet, forward to next hop
        //demote to layer 2 to forward to next hop
        ip_hdr->ttl--;
        if(ip_hdr->ttl ==0){
            //drop pkt
            return;
        }


        /* If route is non direct, then ask LAyer 2 to send the pkt
        * out of all ecmp nexthops of the route*/
        uint32_t next_hop_ip;
        nexthop_t *nexthop = NULL;

        nexthop = l3_route_get_active_nexthop(l3_route);
        assert(nexthop);
    
        inet_pton(AF_INET, nexthop->gw_ip, &next_hop_ip);
        next_hop_ip = htonl(next_hop_ip);
        tcp_dump_l3_fwding_logger(node,nexthop->oif->if_name,nexthop->gw_ip); 

        demote_pkt_to_layer2(node, 
                next_hop_ip,
                nexthop->oif->if_name,
                (char *)ip_hdr, pkt_size,
                ETH_IP); /*Network Layer need to tell Data link layer, 
                       what type of payload it is passing down*/
    }

}



static void _layer3_pkt_recv_from_layer2(node_t *node, interface_t *interface, char *pkt, 
                                            uint32_t pkt_size, int L3_protocol_type){
    switch(L3_protocol_type){
        case ETH_IP:
            layer3_ip_pkt_recv_from_layer2(node,interface,(ip_hdr_t *)pkt,pkt_size);
            break;
        default:
            ;
    }
}



void promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size, int L3_protocol_number){
        _layer3_pkt_recv_from_layer2(node,interface,pkt,pkt_size,L3_protocol_number);
}


static void l3_route_free(l3_route_t *l3_route){
    spf_flush_nexthops(l3_route->nexthops);
    free(l3_route);
}


void clear_rt_table(rt_table_t *rt_table){
    glthread_t *curr;
    l3_route_t *l3_route;

    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list,curr){
        l3_route = rt_glue_to_l3_route(curr);
        if(l3_is_direct_route(l3_route)){
            continue;
        }

        remove_glthread(curr);
        l3_route_free(l3_route);
    }ITERATE_GLTHREAD_END(&rt_table->route_list,curr);
}




void
dump_rt_table(rt_table_t *rt_table){

    int i = 0;
    glthread_t *curr = NULL;
    l3_route_t *l3_route = NULL;
    int count = 0;
    printf("L3 Routing Table:\n");
    ITERATE_GLTHREAD_BEGIN(&rt_table->route_list, curr){

        l3_route = rt_glue_to_l3_route(curr);
        count++;
        if(l3_route->is_direct){
            if(count != 1){
                printf("\t|===================|=======|====================|==============|==========|\n");
            }
            else{
                printf("\t|======= IP ========|== M ==|======== Gw ========|===== Oif ====|== Cost ==|\n");
            }
            printf("\t|%-18s |  %-4d | %-18s | %-12s |          |\n", 
                    l3_route->dest, l3_route->mask, "NA", "NA");
            continue;
        }

        for( i = 0; i < MAX_NXT_HOPS; i++){
            if(l3_route->nexthops[i]){
                if(i == 0){
                    if(count != 1){
                        printf("\t|===================|=======|====================|==============|==========|\n");
                    }
                    else{
                        printf("\t|======= IP ========|== M ==|======== Gw ========|===== Oif ====|== Cost ==|\n");
                    }
                    printf("\t|%-18s |  %-4d | %-18s | %-12s |  %-4u    |\n", 
                            l3_route->dest, l3_route->mask,
                            l3_route->nexthops[i]->gw_ip, 
                            l3_route->nexthops[i]->oif->if_name, l3_route->spf_metric);
                }
                else{
                    printf("\t|                   |       | %-18s | %-12s |          |\n", 
                            l3_route->nexthops[i]->gw_ip, 
                            l3_route->nexthops[i]->oif->if_name);
                }
            }
        }
    } ITERATE_GLTHREAD_END(&rt_table->route_list, curr); 
    printf("\t|===================|=======|====================|==============|==========|\n");
}

