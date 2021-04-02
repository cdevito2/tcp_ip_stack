/*
 * =====================================================================================
 *
 *       Filename:  notif.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-04-02 11:29:26 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __NOTIF_CHAIN__
#define __NOTIF_CHAIN__

#include <stddef.h>
#include "gluethread/glthread.h"
#include "utils.h"

#define MAX_NOTIF_KEY_SIZE 64



typedef void (*nfc_app_cb)(void *, size_t);


typedef struct notif_chain_elem{
    char key[MAX_NOTIF_KEY_SIZE];
    size_t key_size;
    bool_t is_key_set;
    nfc_app_cv app_cb;//callback
    glthread_t glue;//glue to linkedlist

}notif_chain_elan_t;
GLTHREAD_TO_STRUCT(glthread_glue_to_notif_chain_elem,notif_chain_elem_t,glue);

typedef struct notif_chain_{
    char nfc_name[64];
    glthread_t notif_chain_head;//head of linked list
}notif_chain_t;



//subscription request function- register for nfc for events , invoked by subscriber
void nfc_register_notif_chain(notif_chain_t *nfc, notif_chain_elem_t *nfce);

//invoke request
void nfc_invoke_notif_chain(notif_chain_t *nfc, void *arg, size_t arg_size, char *key, size_t key_size);

#endif
