/**************************************************************************
  * @file       : el_simple_brightness_client.h
  * @brief      : create a new model client for control the brightness of light
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/
#ifndef __EL_SIMPLE_BRIGHTNESS_CLIENT_H__
#define __EL_SIMPLE_BRIGHTNESS_CLIENT_H__

#include <stdint.h>
#include <stdbool.h>
#include "access.h"
#include "el_simple_brightness_common.h"

#define EL_SIMPLE_BRIGHTNESS_MODEL_CLIENT_ID (0x0005)

#define EL_SIMPLE_BRIGHTNESS_VALUE_MIN (0)
#define EL_SIMPLE_BRIGHTNESS_VALUE_MAX (100)

typedef enum
{
    EL_SIMPLE_BRIGHTNESS_STATUS_NORMAL,
    EL_SIMPLE_BRIGHTNESS_STATUS_ERROR_NO_REPLY,
    EL_SIMPLE_BRIGHTNESS_STATUS_CANCELLED
} el_simple_brightness_status_t;

typedef struct __el_simple_brightness_client el_simple_brightness_client_t;

typedef void (*el_simple_brightness_status_cb_t)(const el_simple_brightness_client_t * p_self, el_simple_brightness_status_t status, uint16_t src);

typedef void (*el_simple_brightness_timeout_cb_t)(access_model_handle_t handle, void * p_self);

struct __el_simple_brightness_client
{
    access_model_handle_t model_handle;
    el_simple_brightness_status_cb_t status_cb;
    el_simple_brightness_timeout_cb_t timeout_cb;
    /** Internal client state. */
    struct
    {
        bool reliable_transfer_active;
        el_simple_brightness_msg_set_t data;
    }state;
};

uint32_t el_simple_brightness_client_init(el_simple_brightness_client_t * p_client, uint16_t element_index);

uint32_t el_simple_brightness_client_set(el_simple_brightness_client_t * p_client, uint8_t brightness);

uint32_t el_simple_brightness_client_set_unreliable(el_simple_brightness_client_t * p_client, uint8_t brightness, uint8_t repeats);

uint32_t el_simple_brightness_client_get(el_simple_brightness_client_t * p_client);

void el_simple_brightness_client_pending_msg_cancel(el_simple_brightness_client_t * p_client);

#endif //__EL_SIMPLE_BRIGHTNESS_CLIENT_H__