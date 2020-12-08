/*
 * =====================================================================================
 *
 *       Filename:  comm.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20-12-07 08:14:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


/*  include all header files needed for socket programming */

#include "comm.h"
#include "graph.h"
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>



static unsigned int udp_port_number = 40000;

static unsigned int get_next_udp_port(){
    return udp_port_number++;
}

void init_udp_socket(node_t *node){
    //assign udp port number and create socket
    node->udp_port_number = get_next_udp_port_number();

    //create socket

    int udp_sock_fd = socket(AF_INET,SOCK_DRAGM,IPPROTO_UDP);
    //bind socket to 127.0.0.1 + udp port number
    //need to create struct sockaddr_in
    struct sockaddr_in node_addr;
    node_addr.sin_family = AF_INET;//ipv4
    node_addr.sin_port = node->udp_port_number;//want to bind to nodes udp port
    node_addr.sin_addr.s_addr = INNADDR_ANY;//use my IPv4 address
    
    //in bind call need to cast node_addr to sockaddr
    if(bind(idp_sock_fd,(struct sockaddr *)&node_addr,sizeof(struct sockaddr)) == -1){
        printf("Error: socket bind failed for node %s/n",node->node_name);
        return;
    }

    //assign file descriptor to node attribute
    node->udp_sock_fd = udp_sock_fd;

}


static void * _network_start_pkt_receiver_thread(void *arg){
    //iterate over all nodes of topology and using their file descriptors listen on them

    node_t *node;
    glthread_t *curr;
    //store all filedescriptors with fdset
    fd_set active_socket_fd_set;
    fd_set backtive_socket_fd_set;

    int sock_max_fd = 0;
    int bytes_recv = 0;

    grapt_t *topo = (void *)arg;
    //init fd_set to 0

    FD_SET(&active_sock_fd_set);
    FD_SET(&active_sock_fd_set);

    //iterate all nodes and add socket file descriptors to fdset
    struct sockaddr_in sender_addr;


}

void network_start_pkt_receiver_thread(graph_t *topo){
    //invoke thread to monitor file descriptors
    pthread_t recv_pkt_thread;
    //create thread attribute variable
    pthread_t_attr attr;
    //init attribute variable with init function
    pthread_attr_init(&attr);
    //set attribute detach status so it never joins
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    //fork thread
    pthread_create(&pkt_thread,&attr,_network_start_pkt_receiver_thread,(void *)topo);

}












