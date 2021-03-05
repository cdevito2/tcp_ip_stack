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
    ((size_t)&(((spf_dat_t *)0)->priority_thread_glue))


#define SPF_METRIC(nodeptr) (nodeptr->spf_data->spf_metric)


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



void initialize_direct_nbrs(node_t *spf_root){
    //iterate through all nexthops
    node_t *nbr = NULL;
    char *nxt_hop_ip = NULL;
    interface_t *oif = NULL;
    nexthop_t *nexthop = NULL;

    ITERATE_NODE_NBRS_BEGIN(spf_root,nbr,oif,nxt_hop_ip){
        //check if nexthop is eligible(aka bidirectional)
        //if No, go to next nexthop in the list
        if(!is_interface_l3_bidirectional(oif))continue;
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

static void compute_spf(node_t *spf_root){
    //INIT - PART 1
    node_t *node,*nbr;
    interface_t *oif;
    char *nxt_hop_ip = NULL;
    spf_data_t *curr_spf_data;
    glthread_t *curr;
    init_node_spf_data(spf_root,TRUE);
    SPF_METRIC(spf_root) = 0;
    //init metrics
    //delete old spf results if any
    //remove nodes from PQ
    //flush nexthops if any
    

    //iterate through all nodes and call init spf function
    ITERATE_GLTHEAD_BEGIN(&topo->node_list,curr){
        node = graph_glue_to_node(curr);
        if(node==spf_root){
            continue;//init function already called for this node
        }
        init_node_spf_data(node,FALSE);//spf result list only deleted for root node
    }ITERATE_GLTHREAD_END(&topo->node_list,curr);

    //PART 2- compute nexthops
    initialize_direct_nbrs(node_t *spf_root);

    //PART 3 - initialize PQ and add SPF root
    
    glthread_t priority_lst;
    init_glthread(&priority_list);
    glthread_priority_insert(&priority_list,&spf_root->spf_data->priority_thread_glue,spf_comparison_fn,spf_data_offset_from_priority_thread_glue);


    //EXECUTION PHASE
    //



}

static void show_spf_results(node_t *node){
    printf("%s() called ...\n",__FUNCTION__);
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
        default:
            break;
            
    }
    return 0;
}
