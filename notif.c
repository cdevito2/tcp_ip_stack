/*
 * =====================================================================================
 *
 *       Filename:  notif.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-04-02 11:29:21 AM
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
#include <assert.h>
#include "notif.h"

void nfc_register_notif_chain(notif_chain_t *nfc, notif_chain_elem_t *nfce){
    //create copy of nfc element and insert in notification chain
    notif_chain_elem_t *new_nfce = calloc(1,sizeof(notif_chain_elem_t));
    memcpy(new_nfce,nfce,sizeof(notif_chain_elem_t));
    init_glthread(&new_nfce->glue);
    glthread_add_next(&nfc->notif_chain_head, &new_nfce->glue);
}


void nfc_invoke_notif_chain(notif_chain_t *nfc, void *arg, size_t arg_size, char *key, size_t key_size){
    //need to walked the nfc link list
    //for each element it should match key, if match invoke callback

    ///start iteration
    glthread_t *curr;
    notif_chain_elem_t *nfce;
    assert(key_size<= MAX_NOTIF_KEY_SIZE);

    ITERATE_GLTHREAD_BEGIN(&nfc->notif_chain_head,curr){
        nfce = glthread_glue_to_notif_chain_elem(curr);

        if(!(key && key_size && nfce->is_key_set && (key_size == nfce->key_size))){
            //widlcard element
            nfce->app_cb(arg,arg_size);
        }
        else{
            //key match
            if(memcpy(key,nfce->key,key_size) == 0){
                nfce->app_cb(arg,arg_size);
            }

        }
    }ITERATE_GLTHREAD_END(&nfc->notif_chain_head,curr);
}





















