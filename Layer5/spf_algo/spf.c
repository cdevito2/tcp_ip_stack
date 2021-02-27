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
#include "../../tcp_public.h"


extern graph_t *topo;

void compute_spf(node_t *spf_root){
    printf("%s() called ...\n",__FUNCTION__);
}

static void show_spf_results(node_t *node){
    printf("%s() called ...\n",__FUNCTION__);
}



int spf_algo_handler(param_t *param, ser_buff_t *tlv_buff,op_mode enable_or_disable){
    int CMDCODE;
    node_t *node, 
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
