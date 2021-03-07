# tcp_ip_stack

* Created node topologies which included creating virtual L2 Switches, L3 Routers and host machines using structs
* Integrated third party commandline interface into the project allowing networking commands
  - Also created custom commands to add ontop of this library to interact with my network topology


Frame ingress journey in my tcp stack:
1. packet hits pkt_receive function
2. Packet goes to layer2-frame-recv function - point of entry to the stack
3. Packet goes to l2_frame_recv_qualify_on_interface - checks if frame qualifies to be processed by TCP/IP stack(this returns a boolean)
4. If false, discard packet, if true, send to processing/hand over to the stack
4a. if pkt arrived on L3 interface, process pkt by type value( if 806, its an arp msg so layer 2 process. If type is 0x8000 promote to layer3
4b. If pkt arrives on L2 interface, call l2_switch_recv_frame, which feeds to l2 switch forwarding algorithm.


TODO:
-- IP in IP encapsulation
-- Logging infrstructure
-- Notification Chains
-- Integrating wheel timers
