/**************************************************************************
  * @file       : el_simple_brightness_common.h
  * @brief      : define some info for brightness model
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/

#ifndef __EL_SIMPLE_BRIGHTNESS_COMMON_H__
#define __EL_SIMPLE_BRIGHTNESS_COMMON_H__

#include <stdint.h>
#include "access.h"

/** Vendor specific company ID for Simple OnOff model */
#define EL_SIMPLE_BRIGHTNESS_COMPANY_ID (ACCESS_COMPANY_ID_NORDIC)

/** EL Simple Brightness opcodes. */
typedef enum
{
    EL_SIMPLE_BRIGHTNESS_OPCODE_SET = 0xD1,
    EL_SIMPLE_BRIGHTNESS_OPCODE_GET = 0xD2,
    EL_SIMPLE_BRIGHTNESS_OPCODE_SET_UNRELIABLE = 0xD3,
    EL_SIMPLE_BRIGHTNESS_OPCODE_STATUS = 0xD4
} el_simple_brightness_opcode_t;

//typedef enum
//{
//    EL_SIMPLE_BRIGHTNESS_OPCODE_SET = 0x8302,
//    EL_SIMPLE_BRIGHTNESS_OPCODE_GET = 0x8301,
//    EL_SIMPLE_BRIGHTNESS_OPCODE_SET_UNRELIABLE = 0x8303,
//    EL_SIMPLE_BRIGHTNESS_OPCODE_STATUS = 0x8304
//} el_simple_brightness_opcode_t;

typedef struct __attribute((packed))
{
    uint8_t brightness; /**< State to set. */
    uint8_t tid; /**< Transaction number. */
} el_simple_brightness_msg_set_t;

typedef struct __attribute((packed))
{
    uint8_t brightness; /**< State to set. */
    uint8_t tid; /**< Transaction number. */
} el_simple_brightness_msg_set_unreliable_t;

typedef struct __attribute((packed))
{
    uint8_t present_brightness; /**< Current state. */
} el_simple_brightness_msg_status_t;

#endif //__EL_SIMPLE_BRIGHTNESS_COMMON_H__