# tcp_ip_stack

HOW TO USE:

Step 1 : Clone repository

Step 2 : naviagte to testapp.c and chose which network topology you want to create. Implementation of each topology is in the file topology.c

Step 2 : compile project
  -- I have created a makefile for this project which compiles everything. Need to be in a linux enviroment with gcc compiler in order for this to work
 
Step 3 : run executible
  -- executable is called tcpstack.exe
 
The project brings up a CLI in which you can interact with 

Important Commands to know:

-- show topo : shows the all the nodes in the topology and their interface information
-- show node <node-name> arp : shows the ARP table for the specific node
-- show node <node-name> rt : shows the Routing table for the specific node
-- show node <node-name> mac : shows the MAC table for the specific node
-- run node <node-name> ping <ip> : tries to ping the selected ip address. Will use ARP(more explanation below).
-- run node <node-name> resolve-arp <ip-address> : this project has ARP on demand so this cmd is not necesary, but you can still resolve arp manually if you want
  
Features:
-- pkt_gen.exe - will inject packets into a specified note + interface to simulate real traffic in a TCP/IP stack. see pkt_gen.c for details.
-- ON demand ARP - The topology will resolve ARP on demand if you try to ping an IP address, where the current node does not know the destination MAC address. 
-- Dynamic Construction of Routing table - This project runs SPF algorithm on start to determine least cost paths to all node loopback addresses.
-- VLAN based MAC learning/forwarding - There are topologies in topologies.c that consist of multiple switches and VLAN definitions for Switch Ports. Should still be able to      ping interfaces in different VLAns. 


TODO:
-- IP in IP encapsulation
-- Logging infrstructure(ex tcp_dump)
-- Notification Chains
-- Integrating wheel timers
