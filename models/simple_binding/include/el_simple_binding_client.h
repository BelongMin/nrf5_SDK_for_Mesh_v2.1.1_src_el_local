/**************************************************************************
  * @file       : el_simple_binding_client.h
  * @brief      : create a new model client for control the binding of light
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/
#ifndef __EL_SIMPLE_BINDING_CLIENT_H__
#define __EL_SIMPLE_BINDING_CLIENT_H__

#include <stdint.h>
#include <stdbool.h>
#include "access.h"
#include "el_simple_binding_common.h"

#define EL_SIMPLE_BINDING_MODEL_CLIENT_ID (0x0009)

#define EL_SIMPLE_BINDING_VALUE_MIN (0)
#define EL_SIMPLE_BINDING_VALUE_MAX (100)

typedef enum
{
    EL_SIMPLE_BINDING_STATUS_NORMAL,
    EL_SIMPLE_BINDING_STATUS_ERROR_NO_REPLY,
    EL_SIMPLE_BINDING_STATUS_CANCELLED
} el_simple_binding_status_t;

typedef struct __el_simple_binding_client el_simple_binding_client_t;

typedef void (*el_simple_binding_status_cb_t)(const el_simple_binding_client_t * p_self, el_simple_binding_status_t status, uint16_t src);

typedef void (*el_simple_binding_timeout_cb_t)(access_model_handle_t handle, void * p_self);

struct __el_simple_binding_client
{
    access_model_handle_t model_handle;
    el_simple_binding_status_cb_t status_cb;
    el_simple_binding_timeout_cb_t timeout_cb;
    /** Internal client state. */
    struct
    {
        bool reliable_transfer_active;
        el_simple_binding_msg_set_t data;
    }state;
};

uint32_t el_simple_binding_client_init(el_simple_binding_client_t * p_client, uint16_t element_index);

uint32_t el_simple_binding_client_set(el_simple_binding_client_t * p_client, el_simple_binding_data_t binding_data);

uint32_t el_simple_binding_client_set_unreliable(el_simple_binding_client_t * p_client, el_simple_binding_data_t binding_data, uint8_t repeats);

uint32_t el_simple_binding_client_get(el_simple_binding_client_t * p_client);

void el_simple_binding_client_pending_msg_cancel(el_simple_binding_client_t * p_client);

#endif //__EL_SIMPLE_BINDING_CLIENT_H__