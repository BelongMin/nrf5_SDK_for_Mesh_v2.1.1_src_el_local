/* Copyright (c) 2010 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

#include "boards.h"
#include "simple_hal.h"
#include "log.h"
#include "access_config.h"
#include "simple_on_off_client.h"
#include "el_simple_brightness_client.h"
#include "el_simple_color_temperature_client.h"
#include "el_simple_binding_client.h"
#include "rtt_input.h"
#include "device_state_manager.h"
#include "light_switch_example_common.h"
#include "mesh_app_utils.h"
#include "mesh_stack.h"
#include "mesh_softdevice_init.h"
#include "mesh_provisionee.h"
#include "nrf_mesh_config_examples.h"
#include "nrf_mesh_configure.h"
#include "app_timer.h"


#define RTT_INPUT_POLL_PERIOD_MS (100)
#define GROUP_MSG_REPEAT_COUNT   (2)

#define LED_BLINK_INTERVAL_MS       (200)
#define LED_BLINK_SHORT_INTERVAL_MS (50)
#define LED_BLINK_CNT_START         (2)
#define LED_BLINK_CNT_RESET         (3)
#define LED_BLINK_CNT_PROV          (4)
#define LED_BLINK_CNT_NO_REPLY      (6)

//static simple_on_off_client_t m_clients[CLIENT_MODEL_INSTANCE_COUNT];
static simple_on_off_client_t m_on_off_client;
static el_simple_brightness_client_t m_brightness_client;
static el_simple_color_temperature_client_t m_color_temperature_client;
static el_simple_binding_client_t m_binding_client;
static const uint8_t          m_client_node_uuid[NRF_MESH_UUID_SIZE] = CLIENT_NODE_UUID;
static bool                   m_device_provisioned;

static uint8_t m_current_on_off, m_current_brightness, m_current_color_temperature;

static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
}

//static uint32_t server_index_get(const simple_on_off_client_t * p_client)
//{
//    uint32_t index = p_client - &m_clients[0];
//    NRF_MESH_ASSERT(index < SERVER_NODE_COUNT);
//    return index;
//}

static void client_publish_timeout_cb(access_model_handle_t handle, void * p_self)
{
     __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Acknowledged send timedout\n");
}

static void client_status_cb(const simple_on_off_client_t * p_self, simple_on_off_status_t status, uint16_t src)
{
//    uint32_t server_index = server_index_get(p_self);

    switch (status)
    {
        case SIMPLE_ON_OFF_STATUS_ON:
//            hal_led_pin_set(BSP_LED_0 + server_index, true);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OnOff server status ON\n");
            break;

        case SIMPLE_ON_OFF_STATUS_OFF:
//            hal_led_pin_set(BSP_LED_0 + server_index, false);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "OnOff server status OFF\n");
            break;

        case SIMPLE_ON_OFF_STATUS_ERROR_NO_REPLY:
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "No reply from OnOff server\n");
            break;

        case SIMPLE_ON_OFF_STATUS_CANCELLED:
            __LOG(LOG_SRC_APP, LOG_LEVEL_WARN, "Message to server cancelled\n");
            break;
        default:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Unknown status \n");
            break;
    }
}

static void brightness_client_status_cb(const el_simple_brightness_client_t * p_self, el_simple_brightness_status_t status, uint16_t src)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Brightness server status received <---\n");
    switch (status)
    {
        case EL_SIMPLE_BRIGHTNESS_STATUS_NORMAL:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Brightness server status: EL_SIMPLE_BRIGHTNESS_STATUS_NORMAL <---\n");
            break;

        case EL_SIMPLE_BRIGHTNESS_STATUS_ERROR_NO_REPLY:
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            break;

        case EL_SIMPLE_BRIGHTNESS_STATUS_CANCELLED:
        default:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Unknown status \n");
            break;
    }
}

static void color_temperature_client_status_cb(const el_simple_color_temperature_client_t * p_self, el_simple_color_temperature_status_t status, uint16_t src)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Color Temperature server status received <---\n");
    switch (status)
    {
        case EL_SIMPLE_COLOR_TEMPERATURE_STATUS_NORMAL:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Color Temperature server status: EL_SIMPLE_BRIGHTNESS_STATUS_NORMAL <---\n");
            break;

        case EL_SIMPLE_COLOR_TEMPERATURE_STATUS_ERROR_NO_REPLY:
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            break;

        case EL_SIMPLE_COLOR_TEMPERATURE_STATUS_CANCELLED:
        default:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Unknown status \n");
            break;
    }
}

static void binding_client_status_cb(const el_simple_binding_client_t * p_self, el_simple_binding_status_t status, uint16_t src)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Binding server status received <---\n");
    switch (status)
    {
        case EL_SIMPLE_BINDING_STATUS_NORMAL:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Binding server status: EL_SIMPLE_BRIGHTNESS_STATUS_NORMAL <---\n");
            break;

        case EL_SIMPLE_BINDING_STATUS_ERROR_NO_REPLY:
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            break;

        case EL_SIMPLE_BINDING_STATUS_CANCELLED:
        default:
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "Unknown status \n");
            break;
    }
}

static void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_RESET);
    /* This function may return if there are ongoing flash operations. */
    mesh_stack_device_reset();
}

static void config_server_evt_cb(const config_server_evt_t * p_evt)
{
    if (p_evt->type == CONFIG_SERVER_EVT_NODE_RESET)
    {
        node_reset();
    }
}

static void button_event_handler(uint32_t button_number)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Button %u pressed\n", button_number);

    uint32_t status = NRF_SUCCESS;
    el_simple_binding_data_t node_data = {0, 0};
    dsm_local_unicast_address_t node_address;
    switch (button_number)
    {
        case 0:// On Off
            m_current_on_off = !m_current_on_off;  
            status = simple_on_off_client_set_unreliable(&m_on_off_client, m_current_on_off, GROUP_MSG_REPEAT_COUNT);
            break;
        case 1: /* Send Binding Info */
        {
            dsm_local_unicast_addresses_get(&node_address);
            node_data.binding_type = EL_BINDING_TYPE_TX;
            node_data.binding_addr = node_address.address_start;
            status = el_simple_binding_client_set(&m_binding_client, node_data);
            break;
        }

        case 2: /* Initiate node reset */
        {
            /* Clear all the states to reset the node. */
            mesh_stack_config_clear();
            node_reset();
            break;
        }
        case 3:// Brightness Add
            m_current_brightness += 10;
            if(m_current_brightness > LED_DUTY_MAX) m_current_brightness = LED_DUTY_MAX;
            status = el_simple_brightness_client_set_unreliable(&m_brightness_client, m_current_brightness, GROUP_MSG_REPEAT_COUNT);
            break;
        case 4:// Brightness Reduce
            m_current_brightness -= 10;
            if(m_current_brightness < (LED_DUTY_MIN + 10)) m_current_brightness = (LED_DUTY_MIN + 10);
            status = el_simple_brightness_client_set_unreliable(&m_brightness_client, m_current_brightness, GROUP_MSG_REPEAT_COUNT);
            break;
        case 5:// Color Temperature Add
            m_current_color_temperature += 10;
            if(m_current_color_temperature > LED_DUTY_MAX) m_current_color_temperature = LED_DUTY_MAX;
            status = el_simple_color_temperature_client_set_unreliable(&m_color_temperature_client, m_current_color_temperature, GROUP_MSG_REPEAT_COUNT);
            break;
        case 6:// Color Temperature Reduce
            if(m_current_color_temperature >= (LED_DUTY_MIN + 10)) m_current_color_temperature -= 10;
            status = el_simple_color_temperature_client_set_unreliable(&m_color_temperature_client, m_current_color_temperature, GROUP_MSG_REPEAT_COUNT);
            break;
        case 7: // Reserve
            
            break;
        default:
            break;
    }

    switch (status)
    {
        case NRF_SUCCESS:
            break;

        case NRF_ERROR_NO_MEM:
        case NRF_ERROR_BUSY:
        case NRF_ERROR_INVALID_STATE:
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Cannot send - client %u is busy\n", button_number);
            hal_led_blink_ms(LEDS_MASK, LED_BLINK_SHORT_INTERVAL_MS, LED_BLINK_CNT_NO_REPLY);
            break;

        case NRF_ERROR_INVALID_PARAM:
            /* Publication not enabled for this client. One (or more) of the following is wrong:
             * - An application key is missing, or there is no application key bound to the model
             * - The client does not have its publication state set
             *
             * It is the provisioner that adds an application key, binds it to the model and sets
             * the model's publication state.
             */
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Publication not configured for client %u\n", button_number);
            break;

        default:
            ERROR_CHECK(status);
            break;
    }
}

static void rtt_input_handler(int key)
{
    if (key >= '0' && key <= '3')
    {
        uint32_t button_number = key - '0';
        button_event_handler(button_number);
    }
}

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");

//    for (uint32_t i = 0; i < CLIENT_MODEL_INSTANCE_COUNT; ++i)
//    {
        m_on_off_client.status_cb = client_status_cb;
        m_on_off_client.timeout_cb = client_publish_timeout_cb;
        ERROR_CHECK(simple_on_off_client_init(&m_on_off_client, 1));
        ERROR_CHECK(access_model_subscription_list_alloc(m_on_off_client.model_handle));
//    }

    m_brightness_client.status_cb = brightness_client_status_cb;
    m_brightness_client.timeout_cb = client_publish_timeout_cb;
    ERROR_CHECK(el_simple_brightness_client_init(&m_brightness_client, 2));
    ERROR_CHECK(access_model_subscription_list_alloc(m_brightness_client.model_handle));

    m_color_temperature_client.status_cb = color_temperature_client_status_cb;
    m_color_temperature_client.timeout_cb = client_publish_timeout_cb;
    ERROR_CHECK(el_simple_color_temperature_client_init(&m_color_temperature_client, 3));
    ERROR_CHECK(access_model_subscription_list_alloc(m_color_temperature_client.model_handle));

    m_binding_client.status_cb = binding_client_status_cb;
    m_binding_client.timeout_cb = client_publish_timeout_cb;
    ERROR_CHECK(el_simple_binding_client_init(&m_binding_client, 0));
    ERROR_CHECK(access_model_subscription_list_alloc(m_binding_client.model_handle));
}

static void mesh_init(void)
{
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = m_client_node_uuid,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &m_device_provisioned));
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Client Demo -----\n");

    ERROR_CHECK(app_timer_init());
    hal_leds_init();

#if BUTTON_BOARD
    ERROR_CHECK(hal_buttons_init(button_event_handler));
#endif
    m_current_on_off = 1;
    m_current_brightness = 50;
    m_current_color_temperature = 50;

    nrf_clock_lf_cfg_t lfc_cfg = DEV_BOARD_LF_CLK_CFG;
    ERROR_CHECK(mesh_softdevice_init(lfc_cfg));
    mesh_init();
}

static void start(void)
{
    rtt_input_enable(rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);
    ERROR_CHECK(mesh_stack_start());

    if (!m_device_provisioned)
    {
        static const uint8_t static_auth_data[NRF_MESH_KEY_SIZE] = STATIC_AUTH_DATA;
        mesh_provisionee_start_params_t prov_start_params =
        {
            .p_static_data    = static_auth_data,
            .prov_complete_cb = provisioning_complete_cb,
            .p_device_uri = NULL
        };
        ERROR_CHECK(mesh_provisionee_prov_start(&prov_start_params));
    }

    const uint8_t *p_uuid = nrf_mesh_configure_device_uuid_get();
    __LOG_XB(LOG_SRC_APP, LOG_LEVEL_INFO, "Device UUID ", p_uuid, NRF_MESH_UUID_SIZE);

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);
}

int main(void)
{
    initialize();
    execution_start(start);

    for (;;)
    {
        (void)sd_app_evt_wait();
    }
}
