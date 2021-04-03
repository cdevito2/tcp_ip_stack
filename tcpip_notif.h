/*
 * =====================================================================================
 *
 *       Filename:  tcpip_notif.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-04-02 11:53:35 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __TCPIP_NOTIF_C
#define __tCPIP_NOTIF_C

#include "notif.h"
#include "net.h"
/*  
typedef struct intf_nw_props_ intf_nw_props_t;
typedef struct interface_ interface_t;
*/
/* 
 * Structures for interface events notification
 * to subscribers 
 */

typedef struct intf_notif_data_{

	interface_t *interface;
	intf_nw_props_t *old_intf_nw_props;
	uint32_t change_flags;
} intf_notif_data_t;



//routine for interface notif chain
void nfc_intf_register_for_events(nfc_app_cb app_cb);


void
nfc_intf_invoke_notification_to_sbscribers(
	interface_t *intf,
    intf_nw_props_t *old_intf_nw_props,
    uint32_t change_flags);


#endif
