/*
 * =====================================================================================
 *
 *       Filename:  testapp.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-11-27 06:37:26 PM
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
#include "CommandParser/libcli.h"
extern graph_t *build_first_topo();
extern graph_t *build_linear_topo();
extern void nw_init_cli();
//define global variable
graph_t *topo = NULL;

int 
main(int argc, char **argv){
    nw_init_cli();
    topo = build_first_topo();
 //   topo= build_linear_topo();





    //give some time for receiver thread to start
    sleep(4);
    node_t *node = get_node_by_node_name(topo,"R0_re");
    interface_t *oif = get_node_if_by_name(node,"eth0/0");
    char msg[] = "Hello how re you \0";
    send_pkt_out(msg,strlen(msg),oif);
    start_shell();//this call starts the libcli library
    return 0;
}
