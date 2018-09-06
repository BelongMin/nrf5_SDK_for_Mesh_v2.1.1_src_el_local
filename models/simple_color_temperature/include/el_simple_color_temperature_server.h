/**************************************************************************
  * @file       : el_simple_color_temperature_server.h
  * @brief      : create a new model server for control the color_temperature of light
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/

#ifndef __EL_SIMPLE_COLOR_TEMPERATURE_SERVER_H__
#define __EL_SIMPLE_COLOR_TEMPERATURE_SERVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "access.h"

#define EL_SIMPLE_COLOR_TEMPERATURE_SERVER_MODLE_ID (0x0006)
//#define EL_SIMPLE_COLOR_TEMPERATURE_SERVER_MODLE_ID (0x1002)

typedef struct __el_simple_color_temperature_server el_simple_color_temperature_server_t;

typedef uint8_t (*el_simple_color_temperature_get_cb_t)(const el_simple_color_temperature_server_t * p_self);

typedef uint8_t (*el_simple_color_temperature_set_cb_t)(const el_simple_color_temperature_server_t * p_self, uint8_t color_temperature);

struct __el_simple_color_temperature_server
{
    access_model_handle_t model_handle;
    el_simple_color_temperature_get_cb_t get_cb;
    el_simple_color_temperature_set_cb_t set_cb;
};

uint32_t el_simple_color_temperature_server_init(el_simple_color_temperature_server_t * p_server, uint16_t element_index);

uint32_t el_simple_color_temperature_server_status_publish(el_simple_color_temperature_server_t *p_server, uint8_t value);

#endif //__EL_SIMPLE_COLOR_TEMPERATURE_SERVER_H__