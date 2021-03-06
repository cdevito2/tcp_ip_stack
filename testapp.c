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

extern void init_tcp_ip_stack();

extern graph_t *build_first_topo();
extern graph_t *build_linear_topo();
extern graph_t *build_simple_l2_switch_topo();
extern graph_t *build_dualswitch_topo();
extern graph_t *linear_3_node_topo();
extern graph_t *build_square_topo();
extern void nw_init_cli();
//define global variable
graph_t *topo = NULL;

int 
main(int argc, char **argv){
    nw_init_cli();
    //topo = build_dualswitch_topo();
    topo = build_square_topo();



    init_tcp_ip_stack();

    //give some time for receiver thread to start
    start_shell();//this call starts the libcli library
    return 0;
}
