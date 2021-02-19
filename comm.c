/*
 * =====================================================================================
 *
 *       Filename:  comm.c
 *
 *    Description:  Funcitons which implement node communication
 *
 *        Version:  1.0
 *        Created:  20-12-07 08:14:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
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
#include "Layer2/layer2.h"
#include "net.h"

static unsigned int udp_port_number = 40000;



/* **********************************************************************************
 * This function is an internal function which actually sends the data from the socket
 * Flow:
 *      Creates a sockaddr struct with the required information.
 *      Calls the sendto function which sends to the file descriptor specified.
 * Args:
 *      sock_fd - the filedescriptor of the socket to send to
 *      pkt_data - the pkt to send out of the socket
 *      pkt_size - the size of the pkt
 *      dest_udp_port_no - the port number of the recv node 
 * **********************************************************************************     */
static int _send_pkt_out(int sock_fd, char *pkt_data, unsigned int pkt_size, unsigned int dst_udp_port_no){
    //need to create sockaddr struct and invoke sendto
    int rc;
    struct sockaddr_in dest_addr;
    //define the destination for this packet
    dest_addr.sin_family = AF_INET;//ipv4
    dest_addr.sin_port = dst_udp_port_no;
    struct hostent *host = (struct hostent *)gethostbyname("127.0.0.1");
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);
    //now we have defined the dest address now send, note function that calls this function has opened a socket already
    rc = sendto(sock_fd,pkt_data,pkt_size,0,(struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    return rc;

}




//this is internal function hence i made it static
static void _pkt_receive(node_t *receiving_node, char *pkt_with_aux_data, unsigned int pkt_size){
    //segregate interface name from rest of data
    char *recv_intf_name = pkt_with_aux_data;
    interface_t *recv_intf = get_node_if_by_name(receiving_node, recv_intf_name);
    //error check
    if(!recv_intf){
        printf("Error: Pkt received on unknown unterface %s on node %s\n",recv_intf->if_name,receiving_node->node_name);
    }
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)(pkt_with_aux_data+IF_NAME_SIZE);
    //invoke call to receive packet
    //note that pkt_with_aux_data right+shitfs the packet pointer to the start of data
    pkt_receive(receiving_node,recv_intf,pkt_with_aux_data+IF_NAME_SIZE,pkt_size - IF_NAME_SIZE);
}





static unsigned int get_next_udp_port(){
    return udp_port_number++;
}





//import external function from layer2.c 
extern void layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size);







static char recv_buffer[MAX_PACKET_BUFFER_SIZE];//portion of memory used to receive data
static char send_buffer[MAX_PACKET_BUFFER_SIZE];//portion of memory used to receive data

/*  This funciton is called when an interface receives a packet
 *  Flow:
 *      logs print msg stating that pkt has been recvd, shifts the
 *      pkt to the start of the data, and calls external function
 *      layer2_frame_recv.
 *  Args:
 *      node - the node which has an interface which recv the pkt
 *      interface - the interface which recv the packet
 *      pkt - the pkt that was recv
 *      pkt_size - the size of the pkt that was recv */

int pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){
    //entry point into link layer
    //ingress journey of packet starts from here in the TCP IP stack
    //here we perform a right shift to the packet pointer to the right boundary, right before the pkt data, note previously it was pointing to the beginning of data but we need to add space before it
    ethernet_hdr_t *eth = (ethernet_hdr_t *)pkt;
    pkt = pkt_buffer_shift_right(pkt,pkt_size, MAX_PACKET_BUFFER_SIZE - IF_NAME_SIZE);
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t *)pkt;
    //further processing of packet
    ///*  THIS FUNCTION IS THE ENTRY POINT INTO THE TCP IP STACK FROM THE BOTTOM! */
    //note that packet pointer at this point is pointing to the data in the already right shifted packet buffer 
    layer2_frame_recv(node,interface,pkt,pkt_size);
    return 0;
    
}




int
send_pkt_out(char *pkt, unsigned int pkt_size, 
             interface_t *interface){
    printf("SENDING PKT\n");
    int rc = 0;
    node_t *sending_node = interface->att_node;
    node_t *nbr_node = get_nbr_node(interface);
    
    if(!nbr_node)
        return -1;

    if(pkt_size + IF_NAME_SIZE > MAX_PACKET_BUFFER_SIZE){
        printf("Error : Node :%s, Pkt Size exceeded\n", sending_node->node_name);
        return -1;
    }

    unsigned int dst_udp_port_no = nbr_node->udp_port_number;
    
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if(sock < 0){
        printf("Error : Sending socket Creation failed , errno = %d", errno);
        return -1;
    }
    
    interface_t *other_interface = &interface->link->intf1 == interface ? \
                                    &interface->link->intf2 : &interface->link->intf1;

    ethernet_hdr_t *ethhdr = (ethernet_hdr_t *)pkt;
    memset(send_buffer, 0, MAX_PACKET_BUFFER_SIZE);

    char *pkt_with_aux_data = send_buffer;
    strncpy(pkt_with_aux_data, other_interface->if_name, IF_NAME_SIZE);

    pkt_with_aux_data[IF_NAME_SIZE - 1] = '\0';

    memcpy(pkt_with_aux_data + IF_NAME_SIZE, pkt, pkt_size);

    ethernet_hdr_t *eth = (ethernet_hdr_t *)(pkt_with_aux_data + IF_NAME_SIZE);
    rc = _send_pkt_out(sock, pkt_with_aux_data, pkt_size + IF_NAME_SIZE, 
                        dst_udp_port_no);

    close(sock);
    return rc; 
}

/*  
int send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *interface){
    //3rd argument is local interface to send out
    node_t *sending_node = interface->att_node;
    //need pointer to neighbour node
    node_t *nbr_node = get_nbr_node(interface);
    //basic check ensure neightbour node exists
    if(!nbr_node){return -1;}
    ethernet_hdr_t *ethernet_hdr =(ethernet_hdr_t *) pkt;
    printf("ETH TYPE AT BEGGING OF SEND PKT FCN: %u\n",ethernet_hdr->type);
    //get destination port number
    unsigned int dest_udp_port = nbr_node->udp_port_number;
    //unsigned int dest_udp_port = 40001;
    //create socket so you can send data
    int sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    //error check ensure socket created
    if(sock<0){
        printf("Error: socket creation failed, errno = %d",errno);
        return -1;
    }

    //get receiving interface
    interface_t *recv_interface = &interface->link->intf1 == interface ?  &interface->link->intf2 : &interface->link->intf1;

    //initialize buffer to send data
    memset(send_buffer,0,MAX_PACKET_BUFFER_SIZE);

    //prepare data with additional info
    char *pkt_with_aux_data = send_buffer;
    
    //add in receiving interface name
    strncpy(pkt_with_aux_data, recv_interface->if_name, IF_NAME_SIZE);
    pkt_with_aux_data[IF_NAME_SIZE - 1] = '\0';//escape char

    memcpy(pkt_with_aux_data + IF_NAME_SIZE, pkt,pkt_size);

    int rc =0;
    printf("In send pkt out function\n");
    ethernet_hdr_t *eth = (ethernet_hdr_t *)(pkt_with_aux_data + IF_NAME_SIZE);
    printf("ETH HDR RIGHT BEFORE _SEND : %u\n",eth->type);
    rc = _send_pkt_out(sock, pkt_with_aux_data,pkt_size + IF_NAME_SIZE, dest_udp_port);
    close(sock);
    return rc;

}

*/




void init_udp_socket(node_t *node){
    //assign udp port number and create socket
    node->udp_port_number = get_next_udp_port();
    //create socket

    int udp_sock_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    //bind socket to 127.0.0.1 + udp port number
    //need to create struct sockaddr_in
    struct sockaddr_in node_addr;
    node_addr.sin_family = AF_INET;//ipv4
    node_addr.sin_port = node->udp_port_number;//want to bind to nodes udp port
    node_addr.sin_addr.s_addr = INADDR_ANY;//use my IPv4 address
    
    //in bind call need to cast node_addr to sockaddr
    if(bind(udp_sock_fd,(struct sockaddr *)&node_addr,sizeof(struct sockaddr)) == -1){
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
    fd_set active_sock_fd_set;
    fd_set backup_sock_fd_set;

    int sock_max_fd = 0;
    int bytes_recv = 0;
    int addr_len = sizeof(struct sockaddr);
    graph_t *topo = (void *)arg;
    //init fd_set to 0

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    //iterate all nodes and add socket file descriptors to fdset
    struct sockaddr_in sender_addr;
     
    ITERATE_GLTHREAD_BEGIN(&topo->node_list,curr){
        node = graph_glue_to_node(curr);
        if(!node->udp_sock_fd){
            continue;
        }
        //if we hit here then the node has a socket file descriptor
        FD_SET(node->udp_sock_fd,&backup_sock_fd_set);

        //find max value among file descriptors
        if(node->udp_sock_fd > sock_max_fd){
            sock_max_fd = node->udp_sock_fd;
        }
    }ITERATE_GLTHREAD_END(&topo->node_list,curr);

    //start infinite loop and listen all file desriptors
    while(1){
        //add all file descriptors to active set
        memcpy(&active_sock_fd_set,&backup_sock_fd_set, sizeof(fd_set));
        //select function call
        select(sock_max_fd+1,&active_sock_fd_set,NULL,NULL,NULL);
        //this is blocking, so as soon as any data arrives on any FD the code gets executed below
        //if we are here it means 1 or more file descriptors have recv data
        //check which file descriptors have been activated
        ITERATE_GLTHREAD_BEGIN(&topo->node_list,curr){
            //iterate through all nodes
            node = graph_glue_to_node(curr);
            //check each node FD to see if they recv data
            if(FD_ISSET(node->udp_sock_fd, &active_sock_fd_set)){
                //node file descriptor has received data
                //need to receive data so allocate space
                memset(recv_buffer,0,MAX_PACKET_BUFFER_SIZE);
                //execute recvfrom api call
                bytes_recv = recvfrom(node->udp_sock_fd,(char *)recv_buffer,MAX_PACKET_BUFFER_SIZE,0,(struct sockaddr *)&sender_addr,&addr_len);
                //now node has received data from another node, now process it
                _pkt_receive(node, recv_buffer, bytes_recv); 
            }
        }ITERATE_GLTHREAD_END(&topo->node_list,curr);
    }

}








int send_pkt_flood(node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size){
    //send packet out of all node interfaces except the exempt interface
    //loop through the node list of interface
    unsigned int i =0;
    interface_t *intf;
    for(i=0;i<MAX_INTF_PER_NODE;i++){
        intf = node->intf[i];
        if(!intf){
            return 0;
        }
        
        if(intf == exempted_intf){
            continue;
        }

        //if we make it here for an interface then we want to invoke send packet function
        send_pkt_out(pkt,pkt_size,intf);



    }
}





void network_start_pkt_receiver_thread(graph_t *topo){
    //invoke thread to monitor file descriptors
    pthread_t recv_pkt_thread;
    //create thread attribute variable
    pthread_attr_t attr;
    //init attribute variable with init function
    pthread_attr_init(&attr);
    //set attribute detach status so it never joins
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    //fork thread
    pthread_create(&recv_pkt_thread,&attr,
        _network_start_pkt_receiver_thread,(void *)topo);

}











