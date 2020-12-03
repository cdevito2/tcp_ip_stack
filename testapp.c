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
extern graph_t *build_first_topo();

int 
main(int argc, char **argv){
    graph_t *topo = build_first_topo();
    dump_nw_graph(topo);
    return 0;
}
