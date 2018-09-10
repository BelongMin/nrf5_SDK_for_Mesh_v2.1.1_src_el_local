/**************************************************************************
  * @file       : el_simple_binding_server.h
  * @brief      : create a new model server for control the binding of light
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/

#ifndef __EL_SIMPLE_BINDING_SERVER_H__
#define __EL_SIMPLE_BINDING_SERVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "access.h"
#include "el_simple_binding_common.h"

#define EL_SIMPLE_BINDING_SERVER_MODLE_ID (0x0008)

typedef struct __el_simple_binding_server el_simple_binding_server_t;

typedef el_simple_binding_data_t (*el_simple_binding_get_cb_t)(const el_simple_binding_server_t * p_self);

typedef el_simple_binding_data_t (*el_simple_binding_set_cb_t)(const el_simple_binding_server_t * p_self, el_simple_binding_data_t binding_data);

struct __el_simple_binding_server
{
    access_model_handle_t model_handle;
    el_simple_binding_get_cb_t get_cb;
    el_simple_binding_set_cb_t set_cb;
};

uint32_t el_simple_binding_server_init(el_simple_binding_server_t * p_server, uint16_t element_index);

uint32_t el_simple_binding_server_status_publish(el_simple_binding_server_t *p_server, el_simple_binding_data_t binding_data);

#endif //__EL_SIMPLE_BINDING_SERVER_H__