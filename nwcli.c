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




