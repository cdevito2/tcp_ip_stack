/*
 * =====================================================================================
 *
 *       Filename:  cmdcodes.h
 *
 *    Description: Contains CMD codes for the CLI library 
 *
 *        Version:  1.0
 *        Created:  20-12-05 03:08:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __CMDCODES__
#define __CMDCODES__

#define CMDCODE_SHOW_NW_TOPOLOGY     1
#define CMDCODE_PING                 2
#define CMDCODE_RUN_ARP              4
#define CMDCODE_SHOW_NODE_ARP_TABLE  3

#define CMDCODE_INTF_CONFIG_L2_MODE 5   /*config node <node-name> interface <intf-name> l2mode <access|trunk>*/
#define CMDCODE_INTF_CONFIG_IP_ADDR 6   /*config node <node-name> interface <intf-name> ip-address <ip-address> <mask>*/
#define CMDCODE_INTF_CONFIG_VLAN    7   /*config node <node-name> interface <intf-name> vlan <vlan-id>*/
#define CMDCODE_SHOW_NODE_MAC_TABLE  8

#define CMDCODE_SHOW_NODE_RT_TABLE  9   /*show node <node-name> rt*/
#define CMDCODE_CONF_NODE_L3ROUTE   10  /*config node <node-name> route <ip-address> <mask> [<gw-ip> <oif>]*/



#define CMDCODE_SHOW_INTF_STATS     13     /*show node <node-name> interface statistics*/
#define CMDCODE_CONF_INTF_UP_DOWN        26 /*config node <node-name> interface <if-name> <up|down>*/
#endif
