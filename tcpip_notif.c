/*
 * =====================================================================================
 *
 *       Filename:  tcpip_notif.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-04-02 11:59:22 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <memory.h>
#include "net.h"
#include "tcpip_notif.h"


//notif chain for interfaces - config change notification to applications
static notif_chain_t nfc_intf = {
    "Notif Chain for Interfaces",
    {0,0}
};



void nfc_intf_register_for_events(nfc_app_cb app_cb){
    //creat the nfc element
    notif_chain_elem_t nfce_template;

    memset(&nfce_template,0,sizeof(notif_chain_elem_t));

    nfce_template.app_cb = app_cb;
    nfce_template.is_key_set = FALSE;//wildcard entry
    init_glthread(&nfce_template.glue);
    nfc_register_notif_chain(&nfc_intf, &nfce_template);

}


void nfc_intf_invoke_notification_to_subscribers(interface_t *intf, intf_nw_props_t *old_intf_nw_props, uint32_t change_flags){
    
    //pass the arguments into an object
    intf_notif_data_t intf_notif_data;
    intf_notif_data.interface = intf;
    intf_notif_data.old_intf_nw_props = old_intf_nw_props;
    intf_notif_data.change_flags = change_flags;

    nfc_invoke_notif_chain(&nfc_intf, (void *)&intf_notif_data,sizeof(intf_notif_data_t),0,0);

    //key is 0 so all subscribers receive the data

}
























