/*
 * =====================================================================================
 *
 *       Filename:  glthread.c
 *
 *    Description:  implementation of glue based linked list
 *
 *        Version:  1.0
 *        Created:  20-11-29 02:31:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */
#include "glthread.h"
#include <stdlib.h>

void init_glthread(glthread_t *glthread){
    glthread->left = NULL;
    glthread->right = NULL;
}

void glthread_add_next(glthread_t *curr_glthread, glthread_t *new_glthread){
    //iterate to base glthread
    //add new node to the right of it 
    if(!curr_glthread->right){
        //we are at the end of the list so its the last element
        curr_glthread->right = new_glthread;
        new_glthread->right = NULL;
        new_glthread->left = curr_glthread;
        return;
    }
    //we are not at the end of the list so need to sandwich new node in between 2 existing node
    glthread_t *temp = curr_glthread->right;
    //current node has a new node to the right
    curr_glthread->right = new_glthread;
    //new node's left node is the current node
    new_glthread->left = curr_glthread;
    //new node's right node is the previous right node of the current node
    new_glthread->right = temp;
    //temp's new left node is the just added node
    temp->left = new_glthread;
}

void glthread_add_before(glthread_t *curr_glthread, glthread_t *new_glthread){
    if(!curr_glthread->left){
        //new node will be first node in the list
        curr_glthread->left = new_glthread;
        new_glthread->right = curr_glthread;
        new_glthread->left = NULL;
        return;
    }
    //current node already has a previous node so we have to insert
    glthread_t *temp = curr_glthread->left;
    curr_glthread->left = new_glthread;
    new_glthread->right = curr_glthread;
    temp->right = new_glthread;
    new_glthread->left = temp;
}


void remove_glthread(glthread_t *curr_glthread){
    //check if its the front of the list
    if(!curr_glthread->left){
        //its the front of the list
        if(curr_glthread->right){
            //move element to the right to current node poisition
            curr_glthread->right->left = NULL;
            curr_glthread->right = 0;
            return;
        }
        return;
    }

    if(!curr_glthread->right){
        //either at end of list or its the only element in the list
        curr_glthread->left->right = NULL;
        curr_glthread->left->right = NULL;
        return;
    }

    //if we reach here that means that the current node is in between 2 nodes
    curr_glthread->right->left = curr_glthread->left;
    curr_glthread->left->right = curr_glthread->right;
    curr_glthread->right = 0;
    curr_glthread->left = 0 ; 

}


void glthread_add_last(glthread_t *base_glthread, glthread_t *new_glthread){
    //iterate to the last element in the list
    glthread_t *glthreadptr = NULL;
    glthread_t *glthreadprevptr = NULL;
    //use the interator macro that i created
    ITERATE_GLTHREAD_BEGIN(base_glthread,glthreadptr){
        glthreadprevptr = glthreadptr;
    }ITERATE_GLTHREAD_END(base_glthread,glthreadptr);

    //add to next of the last element
    if(glthreadprevptr){
        glthread_add_next(glthreadprevptr,new_glthread);
    }
    //add to next of the first element because there is only one element
    else {
        glthread_add_next(base_glthread,new_glthread);
    }
    
}


void delete_glthread_list(glthread_t *base_glthread){
    //iterate throught the list and delete each one
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread,glthreadptr){
        remove_glthread(glthreadptr);
    }ITERATE_GLTHREAD_END(base_glthread,glthreadptr);
}

unsigned int glthread_list_count(glthread_t *base_glthread){
    //iterate through all elements in the list and increment the count
    //then return it
    unsigned int count = 0;
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread,glthreadptr){
        count++;
    }ITERATE_GLTHREAD_END(base_glthread,glthreadptr);

    return count;

}


