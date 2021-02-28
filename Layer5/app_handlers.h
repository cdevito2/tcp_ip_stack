/*
 * =====================================================================================
 *
 *       Filename:  app_handlers.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  21-02-28 12:35:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __APP_HANDLERS__
#define __APP_HANDLERS__

#include "../CommandParser/libcli.h"

int
spf_algo_handler(param_t *param, ser_buff_t *tlv_buf,
                          op_mode enable_or_disable);


#endif /* __APP_HANDLERS__ */
