/**************************************************************************
  * @file       : el_simple_binding_common.h
  * @brief      : define some info for binding model
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/

#ifndef __EL_SIMPLE_BINDING_COMMON_H__
#define __EL_SIMPLE_BINDING_COMMON_H__

#include <stdint.h>
#include "access.h"

/** Vendor specific company ID for model */
#define EL_SIMPLE_BINDING_COMPANY_ID (ACCESS_COMPANY_ID_NORDIC)

#define EL_BINDING_TYPE_NONE  0x00
#define EL_BINDING_TYPE_TX    0x01
#define EL_BINDING_TYPE_RX    0x02

typedef struct __attribute((packed))
{
    uint8_t binding_type; /**< Type to set. */
    uint16_t binding_addr; /**< Address to set. */
}el_simple_binding_data_t;

/** EL Simple Binding opcodes. */
typedef enum
{
    EL_SIMPLE_BINDING_OPCODE_SET = 0xE1,
    EL_SIMPLE_BINDING_OPCODE_GET = 0xE2,
    EL_SIMPLE_BINDING_OPCODE_SET_UNRELIABLE = 0xE3,
    EL_SIMPLE_BINDING_OPCODE_STATUS = 0xE4
} el_simple_binding_opcode_t;

typedef struct __attribute((packed))
{
    el_simple_binding_data_t binding_data; /**< Binding to set. */
    uint8_t tid; /**< Transaction number. */
} el_simple_binding_msg_set_t;

typedef struct __attribute((packed))
{
    el_simple_binding_data_t binding_data; /**< Binding to set. */
    uint8_t tid; /**< Transaction number. */
} el_simple_binding_msg_set_unreliable_t;

typedef struct __attribute((packed))
{
    el_simple_binding_data_t present_binding_data; /**< Binding to set. */
} el_simple_binding_msg_status_t;

#endif //__EL_SIMPLE_BINDING_COMMON_H__