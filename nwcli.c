/*
 * =====================================================================================
 *
 *       Filename:  nwcli.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-12-05 02:57:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

//include header files for libcli library
#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"

//include header file where we define command codes
#include "cmdcodes.h"
//access graph header files
#include "graph.h"
#include <stdio.h>

extern graph_t *topo; //import global variable from testapp.c

//topo contains info about our network topology

//define handler for show topology
static int show_nw_topology_handler(param_t *param, ser_buff_t *tlv_buff, op_mode enable_or_disable){
    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buff);

    switch(CMDCODE){
        case CMDCODE_SHOW_NW_TOPOLOGY:
            dump_nw_graph(topo);
            break;
        default:
            ;

    }
}

extern void send_arp_broadcast_request(node_t *node, interface_t *oif, char *ip_addr);

//handler for run node resolve arp
//run node <node-name> resolve-arp <ip-address> 
static int arp_handler(param_t *param, ser_buff_t *tlv_buff, op_mode enable_or_disable){
    node_t *node;
    char *node_name;
    char *ip_addr;
    tlv_struct_v *tlv = NULL;

    TLV_LOOP_BEGIN(tlv_buf, tlv){
        if(strncmp(tlv->leaf_if, "node-name", strlen("node-name")) == 0){
            node_name = tlv->value;
        }
        else if(strncmp(tlv->leaf_id,"ip-address",strlen("ip-adress")) ==0){
            ip_addr = tlv->value;
        }
            
    }TLV_LOOP_END;

    node = get_node_by_node_name(topo,node_name);
    send_arp_broadcast_request(node,NULL,ip_addr);
    //2nd arg is null this function should be smart enough to find the outgoing interface
    return 0;


}
void nw_init_cli(){
    //init command line library
    init_libcli();
    //import all the hooks provided by the library
    param_t *show = libcli_get_show_hook();
    param_t *debug = libcli_get_debug_hook();
    param_t *config = libcli_get_config_hook();
    param_t *run = libcli_get_run_hook();
    param_t *debug_show = libcli_get_debug_show_hook();
    param_t *root = libcli_get_root();


    //now i can start creating my custom commands needed for this project
    {
        /* Show topology command */
        //show is already created so build ontop of that
        static param_t topology;
        //add handler
        init_param(&topology, CMD, "topology", show_nw_topology_handler,0, INVALID, 0, "Dump Complete Network Topology");
        //register the parameter to the show hook
        libcli_register_param(show, &topology);
        //set command code
        set_param_cmd_code(&topology, CMDCODE_SHOW_NW_TOPOLOGY);

    }
    //add command negation
    support_cmd_negation(config);

}




