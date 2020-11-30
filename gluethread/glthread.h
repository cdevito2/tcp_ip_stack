/*
 * =====================================================================================
 *
 *       Filename:  glthread.h
 *
 *    Description:  implementation of doubly linked list
 *
 *        Version:  1.0
 *        Created:  20-11-29 02:15:14 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chris Devito
 *   Organization:  
 *
 * =====================================================================================
 */


#ifndef __GLUETHREAD__
#define __GLUETHREAD__

typedef struct _glthread{
    struct _glthread *left; //pointer to the before node
    struct _glthread *right; //pointer to the next node

}glthread_t;

//add new element to linked list
void glthread_add_next(glthread_t *base_glthread, glthread_t *new_glthread);

//add element to before current node
void glthread_add_before(glthread_t *base_glthread, glthread_t *new_glthread);

//remove node
void remove_glthread(glthread_t *glthread);

//init linked list
void init_glthread(glthread_t *glthread);

//add node to end of list
void glthread_add_last(glthread_t *base_glthread, glthread_t *new_glthread);

//delete list
void delete_glthread_list(glthread_t *base_glthread);

//count number of nodes in list
unsigned int glthread_list_count(glthread_t *base_glthread);

//macro functions


#define GLTHREAD_TO_STRUCT(fn_name, structure_name, field_name)                        \
    static inline structure_name * fn_name(glthread_t *glthreadptr){                   \
        return (structure_name *)((char *)(glthreadptr) - (char *)&(((structure_name *)0)->field_name)); \
    }


#define BASE(glthreadptr)   ((glthreadptr)->right)

#define ITERATE_GLTHREAD_BEGIN(glthreadptrstart, glthreadptr)                                      \
{                                                                                                  \
    glthread_t *_glthread_ptr = NULL;                                                              \
    glthreadptr = BASE(glthreadptrstart);                                                          \
    for(; glthreadptr!= NULL; glthreadptr = _glthread_ptr){                                        \
        _glthread_ptr = (glthreadptr)->right;

#define ITERATE_GLTHREAD_END(glthreadptrstart, glthreadptr)                                        \
        }}

#define GLTHREAD_GET_USER_DATA_FROM_OFFSET(glthreadptr, offset)  \
    (void *)((char *)(glthreadptr) - offset)

    

#endif
