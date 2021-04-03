/*
 * =====================================================================================
 *
 *       Filename:  spf.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-26 06:42:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include "../../tcp_public.h"


#define INFINITE_METRIC 0xFFFFFFFF
#define SPF_LOGGING 0


extern graph_t *topo;


typedef struct spf_data_{
   node_t *node; //pointer to owning node
   glthread_t spf_result_head; //valid only for spf root
   /* temp fields */
   uint32_t spf_metric;
   glthread_t priority_thread_glue;
   nexthop_t *nexthops[MAX_NXT_HOPS];

}spf_data_t;
GLTHREAD_TO_STRUCT(priority_thread_glue_to_spf_data,spf_data_t,priority_thread_glue);

typedef struct spf_result_{
    node_t *node;
    uint32_t spf_metric;
    nexthop_t *nexthops[MAX_NXT_HOPS];
    glthread_t spf_res_glue;
}spf_result_t;
GLTHREAD_TO_STRUCT(spf_res_glue_to_spf_result,spf_result_t,spf_res_glue);


#define spf_data_offset_from_priority_thread_glue \
    ((size_t)&(((spf_data_t *)0)->priority_thread_glue))


#define SPF_METRIC(nodeptr) (nodeptr->spf_data->spf_metric)

void spf_flush_nexthops(nexthop_t **nexthop){
    int i=0;
    if(!nexthop){
        return;
    }
    for (;i<MAX_NXT_HOPS;i++){
        if(nexthop[i]){
            assert(nexthop[i]->ref_count);
            nexthop[i]->ref_count--;
            if(nexthop[i]->ref_count ==0){
                free(nexthop[i]);
            }
            nexthop[i] = NULL;
        }
    }
}

static inline void free_spf_result(spf_result_t *spf_result){
    spf_flush_nexthops(spf_result->nexthops);
    remove_glthread(&spf_result->spf_res_glue);
    free(spf_result);
}


static void init_node_spf_data(node_t *node, bool_t delete_spf_result){

    if(!node->spf_data){
        node->spf_data = calloc(1,sizeof(spf_data_t));
        init_glthread(&node->spf_data->spf_result_head);
        node->spf_data->node = node;//back pointer
    }
    else if(delete_spf_result){
        //check all spf results and delete if they exist
        glthread_t *curr;
        ITERATE_GLTHREAD_BEGIN(&node->spf_data->spf_result_head,curr){
            spf_result_t *res = spf_res_glue_to_spf_result(curr);
            free_spf_result(res);
        }ITERATE_GLTHREAD_END(&node->spf_data->spf_result_head,curr);
        init_glthread(&node->spf_data->spf_result_head);
    }

    SPF_METRIC(node) = INFINITE_METRIC;
    remove_glthread(&node->spf_data->priority_thread_glue);
    spf_flush_nexthops(node->spf_data->nexthops);
}




static nexthop_t * create_new_nexthop(interface_t *oif){
    nexthop_t *nexthop = calloc(1,sizeof(nexthop_t));
    nexthop->oif = oif;
    interface_t *other_intf = &oif->link->intf1 == oif ? \
            &oif->link->intf2 : &oif->link->intf1;

    if(!other_intf){
        free(nexthop);
        return NULL;
    }
    strncpy(nexthop->gw_ip, IF_IP(other_intf),16);
    nexthop->ref_count = 0;
    return nexthop;
}


static bool_t spf_insert_new_nexthop(nexthop_t **nexthop_arr, nexthop_t *nxthop){
    //insert second arg into the first arg if possible
    int i=0;
    for(;i<MAX_NXT_HOPS;i++){
        if(nexthop_arr[i]){
            continue;
        }
        nexthop_arr[i] = nxthop;
        nexthop_arr[i]->ref_count++;
        return TRUE;
    }
    return FALSE;
}


static bool_t spf_is_nexthop_exist(nexthop_t **nexthop_arr, nexthop_t *nxthop){
    //returns true if arg 2 exists in arg 1
    int i=0;
    for (; i<MAX_NXT_HOPS;i++){
        if(!nexthop_arr[i]){
            continue;
        }
        if(nexthop_arr[i]->oif == nxthop->oif){
            return TRUE;
        }
    }
    return FALSE;
}


static int spf_union_nexthops_arrays(nexthop_t **src, nexthop_t **dst){
    int i=0;
    int j=0;
    int copied_count=0;
    while(j<MAX_NXT_HOPS && dst[j]){
        j++;
    }

    if(j==MAX_NXT_HOPS){
        return 0;
    }
    
    for (; i<MAX_NXT_HOPS && j<MAX_NXT_HOPS;i++,j++){
        if(src[i] && spf_is_nexthop_exist(dst,src[i]) == FALSE){
            dst[j] = src[i];
            dst[j]->ref_count++;
            copied_count++;
        }

    }
    return copied_count;
}

static int spf_comparison_fn(void *data1, void *data2){
    spf_data_t *spf_data_1 = (spf_data_t *)data1;
    spf_data_t *spf_data_2 = (spf_data_t *)data2;
    if(spf_data_1->spf_metric < spf_data_2->spf_metric){
        return -1;
    }
    if(spf_data_1->spf_metric > spf_data_2->spf_metric){
        return 1;
    }
    return 0;
}


static spf_result_t *spf_lookup_spf_result_by_node(node_t *spf_root, node_t *node){
    //first arg is root node for spf, second arg is some other node in topology
    //iterate thrpugh spf result
    glthread_t *curr;
    spf_result_t *spf_result;
    spf_data_t *curr_spf_data;

    ITERATE_GLTHREAD_BEGIN(&spf_root->spf_data->spf_result_head,curr){
        spf_result = spf_res_glue_to_spf_result(curr);
        if(spf_result->node == node){
            return spf_result;
        }
    }ITERATE_GLTHREAD_END(&spf_root->spf_data->spf_result_head,curr);
    return NULL;

}

static char *
nexthops_str(nexthop_t **nexthops){

    static char buffer[256];
    memset(buffer, 0 , 256);

    int i = 0;

    for( ; i < MAX_NXT_HOPS; i++){

        if(!nexthops[i]) continue;
        snprintf(buffer, 256, "%s ", nexthop_node_name(nexthops[i]));
    }
    return buffer;
}


static void spf_record_result(node_t *spf_root, node_t *processed_node){
    //processed node is the node dequeued
    //shortest path has been calculated for that node
    assert(!spf_lookup_spf_result_by_node(spf_root, processed_node));

    spf_result_t *spf_result = calloc(1,sizeof(spf_result_t));

    //we record the node itself, the shortest path cost to the node
    //and the set of nexthops for this node
    spf_result->node = processed_node;
    spf_result->spf_metric = processed_node->spf_data->spf_metric;

    spf_union_nexthops_arrays(processed_node->spf_data->nexthops,spf_result->nexthops);
    #if SPF_LOGGING
    printf("root : %s : Event : Result Recorded for node %s, "
            "Next hops : %s, spf_metric = %u\n",
            spf_root->node_name, processed_node->node_name,
            nexthops_str(spf_result->nexthops),
            spf_result->spf_metric);
    #endif


    //add the result datastructure for processed node to the spf result table
    init_glthread(&spf_result->spf_res_glue);
    glthread_add_next(&spf_root->spf_data->spf_result_head,&spf_result->spf_res_glue);

}

void initialize_direct_nbrs(node_t *spf_root){
    //iterate through all nexthops
    node_t *nbr = NULL;
    char *nxt_hop_ip = NULL;
    interface_t *oif = NULL;
    nexthop_t *nexthop = NULL;

    ITERATE_NODE_NBRS_BEGIN(spf_root,nbr,oif,nxt_hop_ip){
        //check if nexthop is eligible(aka bidirectional)
        //if No, go to next nexthop in the list
        if(!is_interface_l3_bidirectional(oif)){
            continue;
        }
        //if YESS, check if the cost is less then current
        if(get_link_cost(oif) < SPF_METRIC(nbr)){
            //add the new least cost path
            spf_flush_nexthops(nbr->spf_data->nexthops);
            nexthop = create_new_nexthop(oif);
            spf_insert_new_nexthop(nbr->spf_data->nexthops,nexthop);
            SPF_METRIC(nbr) = get_link_cost(oif);
        }
        //ECMP CASE
        if(get_link_cost(oif) == SPF_METRIC(nbr)){
            //just add the new hop to the nbr 
            nexthop = create_new_nexthop(oif);
            spf_insert_new_nexthop(nbr->spf_data->nexthops,nexthop);
        }
    }ITERATE_NODE_NBRS_END(spf_root,nbr,oif,nxt_hop_ip);
}



static void spf_explore_nbrs(node_t *spf_root, node_t *curr_node, glthread_t *priority_list){
    //curr node is the current node being explored
    node_t *nbr;
    interface_t *oif;
    char *nxt_hop_ip = NULL;
    #if SPF_LOGGING
    printf("root : %s : Event : Nbr Exploration Start for Node : %s\n",
            spf_root->node_name, curr_node->node_name);
    #endif

    //iterate all nbrs of the current node
    ITERATE_NODE_NBRS_BEGIN(curr_node,nbr,oif,nxt_hop_ip){

        #if SPF_LOGGING
        printf("root : %s : Event : For Node %s , Processing nbr %s\n",
                spf_root->node_name, curr_node->node_name, 
                nbr->node_name);
        #endif
        //check if nbr is eligible
        
        if(!is_interface_l3_bidirectional(oif)) continue;

    
        if(SPF_METRIC(curr_node) + get_link_cost(oif) < SPF_METRIC(nbr)){
            //we have found a nbr node reachable by a better cost
            //remove from PQ and add bacl

            //remove obsolete nexthops
            spf_flush_nexthops(nbr->spf_data->nexthops);

            //copy new set of nexthops to nbr
            spf_union_nexthops_arrays(curr_node->spf_data->nexthops,nbr->spf_data->nexthops);

            //update shortest path
            SPF_METRIC(nbr) = SPF_METRIC(curr_node) + get_link_cost(oif);

            //remove from PQ and readd if it is already in PQ
            if(!IS_GLTHREAD_LIST_EMPTY(&nbr->spf_data->priority_thread_glue)){
                #if SPF_LOGGING
                printf("root : %s : Event : Node %s Already On priority Queue\n",
                        spf_root->node_name, nbr->node_name);
                #endif
                remove_glthread(&nbr->spf_data->priority_thread_glue);
            }
            #if SPF_LOGGING
            printf("root : %s : Event : Node %s inserted into priority Queue "
            "with spf_metric = %u\n",
                    spf_root->node_name,  nbr->node_name, nbr->spf_data->spf_metric);
            #endif
            glthread_priority_insert(priority_list, 
                    &nbr->spf_data->priority_thread_glue,
                    spf_comparison_fn, 
                    spf_data_offset_from_priority_thread_glue);
        }


        //case 2 - ECMP CASE
        else if(SPF_METRIC(curr_node) + get_link_cost(oif) == SPF_METRIC(nbr)){
            //combine all netxtophs
            #if SPF_LOGGING
            printf("root : %s : Event : Primary Nexthops Union of Current Node"
                    " %s(%s) with Nbr Node %s(%s)\n",
                    spf_root->node_name,  curr_node->node_name, 
                    nexthops_str(curr_node->spf_data->nexthops),
                    nbr->node_name, nexthops_str(nbr->spf_data->nexthops));
            #endif
            spf_union_nexthops_arrays(curr_node->spf_data->nexthops,nbr->spf_data->nexthops);
        }
    
    }ITERATE_NODE_NBRS_END(curr_node,nbr,oif,nxt_hop_ip);


    #if SPF_LOGGING
    printf("root : %s : Event : Node %s has been processed, nexthops %s\n",
            spf_root->node_name, curr_node->node_name, 
            nexthops_str(curr_node->spf_data->nexthops));
    #endif
    /* We are done processing the curr_node, remove its nexthops to lower the
     * ref count*/
    spf_flush_nexthops(curr_node->spf_data->nexthops); 

}


static int spf_install_routes(node_t *spf_root){
    rt_table_t *rt_table = NODE_RT_TABLE(spf_root);

    //clear all routes except direct ones
    clear_rt_table(rt_table);

    //iterate over spf results and install routes for loopback aadrs of all router nodes

    int i =0;
    int count = 0;
    glthread_t *curr;
    spf_result_t *spf_result;
    nexthop_t *nexthop = NULL;

    ITERATE_GLTHREAD_BEGIN(&spf_root->spf_data->spf_result_head,curr){
        spf_result = spf_res_glue_to_spf_result(curr);
        for(i=0;i<MAX_NXT_HOPS;i++){
            nexthop = spf_result->nexthops[i];
            if(!nexthop)continue;

            //add the route
            rt_table_add_route(rt_table,NODE_LO_ADDR(spf_result->node),32,
            nexthop->gw_ip,nexthop->oif,spf_result->spf_metric);
            count++;
        }
    }ITERATE_GLTHREAD_END(&spf_root->spf_data->spf_result_head,curr);
    return count;
}



static void compute_spf(node_t *spf_root){
    //INIT - PART 1
    node_t *node,*nbr;
    interface_t *oif;
    char *nxt_hop_ip = NULL;
    spf_data_t *curr_spf_data;
    glthread_t *curr;
    
    #if SPF_LOGGING
    printf("root : %s : Event : Running Spf\n", spf_root->node_name);
    #endif
    
    init_node_spf_data(spf_root,TRUE);
    SPF_METRIC(spf_root) = 0;
    //init metrics
    //delete old spf results if any
    //remove nodes from PQ
    //flush nexthops if any
    
    //iterate through all nodes and call init spf function
    ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){

        node = graph_glue_to_node(curr);
        if(node == spf_root) continue;
        init_node_spf_data(node, FALSE);
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);

    //PART 2- compute nexthops
    initialize_direct_nbrs(spf_root);

    //PART 3 - initialize PQ and add SPF root
    
    glthread_t priority_list;
    
    init_glthread(&priority_list);
    
    glthread_priority_insert(&priority_list,
        &spf_root->spf_data->priority_thread_glue,
        spf_comparison_fn,
        spf_data_offset_from_priority_thread_glue);




    //EXECUTION PHASE
    while(!IS_GLTHREAD_LIST_EMPTY(&priority_list)){
        //dequeue first in the queue
        curr = dequeue_glthread_first(&priority_list);
        //check if its the root
        curr_spf_data = priority_thread_glue_to_spf_data(curr);

        #if SPF_LOGGING
        printf("root : %s : Event : Node %s taken out of priority queue\n",
                spf_root->node_name, curr_spf_data->node->node_name);
        #endif

        if(curr_spf_data->node == spf_root){
            //for every eligible nbor not in PQ push to PQ
            ITERATE_NODE_NBRS_BEGIN(curr_spf_data->node,nbr,oif,nxt_hop_ip){
                if(!is_interface_l3_bidirectional(oif))continue;
                if(IS_GLTHREAD_LIST_EMPTY(&nbr->spf_data->priority_thread_glue)){
                    #if SPF_LOGGING
                    printf("root : %s : Event : Processing Direct Nbr %s\n", 
                        spf_root->node_name, nbr->node_name);
                    #endif
                    //add nbr to queue if they are not already in it
                    glthread_priority_insert(&priority_list, 
                            &nbr->spf_data->priority_thread_glue,
                            spf_comparison_fn, 
                            spf_data_offset_from_priority_thread_glue);

                    #if SPF_LOGGING
                    printf("root : %s : Event : Direct Nbr %s added to priority Queue\n",
                            spf_root->node_name, nbr->node_name);
                    #endif
                    
                }
            }ITERATE_NODE_NBRS_END(curr_spf_data->node,nbr,oif,nxt_hop_ip);
            #if SPF_LOGGING
            printf("root : %s : Event : Root %s Processing Finished\n", 
                    spf_root->node_name, curr_spf_data->node->node_name);
            #endif
            continue;
        }

        //step 5- node removed from PQ is not root - record result for node N
        spf_record_result(spf_root,curr_spf_data->node);
        
        //step 6 - explore NBRS of the removed node
        spf_explore_nbrs(spf_root,curr_spf_data->node,&priority_list);
    }






    //step 7 - calculate rputing table from spf result of spf root
    int count = spf_install_routes(spf_root);


    #if SPF_LOGGING
    printf("root : %s : Event : Route Installation Count = %d\n", 
            spf_root->node_name, count);
    #endif
}

static void show_spf_results(node_t *node){
    int i=0,j=0;
    glthread_t *curr;
    interface_t *oif = NULL;
    spf_result_t *res = NULL;
    printf("\nSPF run results for node = %s\n", node->node_name);

    //loop through the nodes spf results list
    ITERATE_GLTHREAD_BEGIN(&node->spf_data->spf_result_head,curr){
        res = spf_res_glue_to_spf_result(curr);

        //print out the information for each node in the spf res linked list
        printf("DEST : %-10s spf_metric : %-6u", res->node->node_name, res->spf_metric);
        printf(" Nxt Hop : ");

        j = 0;
        //loop through all possible nexthops
        for(i=0;i<MAX_NXT_HOPS;i++,j++){
            if(!res->nexthops[i])continue;

            oif = res->nexthops[i]->oif;
            if(j == 0){
                printf("%-8s       OIF : %-7s    gateway : %-16s ref_count = %u\n",
                        nexthop_node_name(res->nexthops[i]),
                        oif->if_name, res->nexthops[i]->gw_ip, 
                        res->nexthops[i]->ref_count);
            }
            else{
                printf("                                              : "
                        "%-8s       OIF : %-7s    gateway : %-16s ref_count = %u\n",
                        nexthop_node_name(res->nexthops[i]),
                        oif->if_name, res->nexthops[i]->gw_ip, 
                        res->nexthops[i]->ref_count);
            }
        }

    }ITERATE_GLTHREAD_END(&node->spf_data->spf_result_head,curr);
}




static void compute_spf_all_routers(graph_t *topo){
    glthread_t *curr;
    ITERATE_GLTHREAD_BEGIN(&topo->node_list,curr){
        node_t *node = graph_glue_to_node(curr);
        compute_spf(node);
    }ITERATE_GLTHREAD_END(&topo->node_list,curr);
}


static void spf_algo_interface_update(void *arg, size_t arg_size){
    intf_notif_data_t *intf_notif_data = (intf_notif_data_t *)arg;
    uint32_t flags = intf_notif_data->change_flags;
    interface_t *intf = intf_notif_data->interface;
    intf_nw_props_t *old_intf_nw_props = intf_notif_data->old_intf_nw_props;

    printf("%s() called\n",__FUNCTION__);

    if(IS_BIT_SET(flags,IF_UP_DOWN_CHANGE_F) || IS_BIT_SET(flags,IF_METRIC_CHANGE_F)){
        goto RUN_SPF;
    }
    return;
RUN_SPF:
    compute_spf_all_routers(topo);
}




void init_spf_algo()
{
    compute_spf_all_routers(topo);
    nfc_intf_register_for_events(spf_algo_interface_update);
}

int spf_algo_handler(param_t *param, ser_buff_t *tlv_buf,op_mode enable_or_disable){
    int CMDCODE;
    node_t *node; 
    char *node_name;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buf);
    tlv_struct_t *tlv = NULL;
    TLV_LOOP_BEGIN(tlv_buf,tlv){
        if(strncmp(tlv->leaf_id,"node-name",strlen("node-name"))==0){
            node_name = tlv->value;
        }
        else{
            assert(0);
        }
    }TLV_LOOP_END;

    if(node_name){
        node = get_node_by_node_name(topo,node_name);
    }

    switch(CMDCODE){
        case CMDCODE_SHOW_SPF_RESULTS:
            show_spf_results(node);
            break;
        case CMDCODE_RUN_SPF:
            compute_spf(node);
            break;
        case CMDCODE_RUN_SPF_ALL:
            compute_spf_all_routers(topo);
        default:
            break;
            
    }
    return 0;
}
