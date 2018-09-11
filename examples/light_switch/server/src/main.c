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

#include "boards.h"
#include "simple_hal.h"
#include "log.h"
#include "access_config.h"
#include "simple_on_off_server.h"
#include "el_simple_brightness_server.h"
#include "el_simple_color_temperature_server.h"
#include "el_simple_binding_client.h"
#include "light_switch_example_common.h"
#include "mesh_app_utils.h"
#include "net_state.h"
#include "rtt_input.h"
#include "mesh_stack.h"
#include "mesh_softdevice_init.h"
#include "mesh_provisionee.h"
#include "nrf_mesh_config_examples.h"
#include "nrf_mesh_configure.h"
#include "app_timer.h"

#include "nrf_mesh_events.h"
#include "nrf_nvic.h"


#define RTT_INPUT_POLL_PERIOD_MS (100)
#define LED_PIN_NUMBER           (BSP_LED_0)
#define LED_PIN_MASK             (1u << LED_PIN_NUMBER)
#define LED_BLINK_INTERVAL_MS    (200)
#define LED_BLINK_CNT_START      (2)
#define LED_BLINK_CNT_RESET      (3)
#define LED_BLINK_CNT_PROV       (4)

static simple_on_off_server_t m_server;
static el_simple_brightness_server_t m_brightness_server;
static el_simple_color_temperature_server_t m_color_temperature_server;
static el_simple_binding_client_t m_binding_client;
//static uint8_t m_device_brightness;
//static uint8_t m_device_color_temperature;
static uint8_t m_current_on_off, m_current_brightness, m_current_color_temperature;

static bool                   m_device_provisioned;

static void led_event_handler(uint8_t on_off, uint8_t brightness, uint8_t color_temperature)
{
    if(on_off > 1 || brightness > LED_DUTY_MAX || color_temperature > LED_DUTY_MAX) return;

    if(on_off == 0)
    {
        hal_pwm_duty_set(LED_COLOR_WARM_CHANNEL, LED_DUTY_MIN);
        hal_pwm_duty_set(LED_COLOR_COLD_CHANNEL, LED_DUTY_MIN);
    }
    else
    {
        uint8_t warm_duty = (uint8_t)((float)color_temperature * (float)brightness/100.0f);
        uint8_t cold_duty = (uint8_t)((float)(100 - color_temperature) * (float)brightness/100.0f);
        hal_pwm_duty_set(LED_COLOR_WARM_CHANNEL, warm_duty);
        hal_pwm_duty_set(LED_COLOR_COLD_CHANNEL, cold_duty);
    }
//    m_current_on_off = on_off;
//    m_current_brightness = brightness;
//    m_current_color_temperature = color_temperature;
}

static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);

    hal_led_mask_set(LEDS_MASK, LED_MASK_STATE_OFF);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
}

static bool on_off_server_get_cb(const simple_on_off_server_t * p_server)
{
    return hal_led_pin_get(LED_PIN_NUMBER);
}

static bool on_off_server_set_cb(const simple_on_off_server_t * p_server, bool value)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Got SET command to %u\n", value);
    hal_led_pin_set(LED_PIN_NUMBER, value);
    m_current_on_off = value;
    led_event_handler(m_current_on_off, m_current_brightness, m_current_color_temperature);
    return value;
}


static uint8_t el_brightness_get_cb(const el_simple_brightness_server_t * p_server)
{
    return m_current_brightness;
}

static uint8_t el_brightness_set_cb(const el_simple_brightness_server_t * p_server, uint8_t value)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "--->Brightness of device has been set to %u<---\n", value);
    
    m_current_brightness = value;
    led_event_handler(m_current_on_off, m_current_brightness, m_current_color_temperature);
    
    return value;
}


static uint8_t el_color_temperature_get_cb(const el_simple_color_temperature_server_t * p_server)
{
    return m_current_color_temperature;
}

static uint8_t el_color_temperature_set_cb(const el_simple_color_temperature_server_t * p_server, uint8_t value)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "--->Color Temperature of device has been set to %u<---\n", value);
    
    m_current_color_temperature = value;
    led_event_handler(m_current_on_off, m_current_brightness, m_current_color_temperature);
    
    return value;
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
            __LOG(LOG_SRC_APP, LOG_LEVEL_ERROR, "--->Binding server status: EL_SIMPLE_BINDING_STATUS_ERROR_NO_REPLY <---\n");
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
    el_simple_binding_data_t node_data = {0, 0};
    dsm_local_unicast_address_t node_address;
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Button %u pressed\n", button_number);
    switch (button_number)
    {
        case 1: /* Send Binding Info */
        {
            dsm_local_unicast_addresses_get(&node_address);
            node_data.binding_type = EL_BINDING_TYPE_RX;
            node_data.binding_addr = node_address.address_start;
            (void)el_simple_binding_client_set(&m_binding_client, node_data);
            break;
        }

        case 2: /* Initiate node reset */
        {
            /* Clear all the states to reset the node. */
            mesh_stack_config_clear();
            node_reset();
            break;
        }
        /* Pressing SW4 on the Development Kit will result in LED state to toggle and trigger
        the STATUS message to inform client about the state change. This is a demonstration of
        state change publication due to local event. */
        case 3:
        {
            uint8_t value = !hal_led_pin_get(LED_PIN_NUMBER);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "User action \n");
            hal_led_pin_set(LED_PIN_NUMBER, value);
            (void)simple_on_off_server_status_publish(&m_server, value);
            break;
        }
        default:
            break;
    }
}

static void app_rtt_input_handler(int key)
{
    if (key >= '0' && key <= '4')
    {
        uint32_t button_number = key - '0';
        button_event_handler(button_number);
    }
}

static void models_init_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Initializing and adding models\n");
    m_server.get_cb = on_off_server_get_cb;
    m_server.set_cb = on_off_server_set_cb;
    ERROR_CHECK(simple_on_off_server_init(&m_server, 0));
    ERROR_CHECK(access_model_subscription_list_alloc(m_server.model_handle));

    m_brightness_server.get_cb = el_brightness_get_cb;
    m_brightness_server.set_cb = el_brightness_set_cb;
    ERROR_CHECK(el_simple_brightness_server_init(&m_brightness_server, 1));
    ERROR_CHECK(access_model_subscription_list_alloc(m_brightness_server.model_handle));

    m_color_temperature_server.get_cb = el_color_temperature_get_cb;
    m_color_temperature_server.set_cb = el_color_temperature_set_cb;
    ERROR_CHECK(el_simple_color_temperature_server_init(&m_color_temperature_server, 2));
    ERROR_CHECK(access_model_subscription_list_alloc(m_color_temperature_server.model_handle));

    m_binding_client.status_cb = binding_client_status_cb;
    m_binding_client.timeout_cb = NULL;
    ERROR_CHECK(el_simple_binding_client_init(&m_binding_client, 0));
    ERROR_CHECK(access_model_subscription_list_alloc(m_binding_client.model_handle));

    hal_led_mask_set(LEDS_MASK, false);
    hal_led_blink_ms(LEDS_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_START);
}

static void mesh_init(void)
{
    uint8_t dev_uuid[NRF_MESH_UUID_SIZE];
    uint8_t node_uuid_prefix[SERVER_NODE_UUID_PREFIX_SIZE] = SERVER_NODE_UUID_PREFIX;

    ERROR_CHECK(mesh_app_uuid_gen(dev_uuid, node_uuid_prefix, SERVER_NODE_UUID_PREFIX_SIZE));
    mesh_stack_init_params_t init_params =
    {
        .core.irq_priority       = NRF_MESH_IRQ_PRIORITY_LOWEST,
        .core.lfclksrc           = DEV_BOARD_LF_CLK_CFG,
        .core.p_uuid             = dev_uuid,
        .models.models_init_cb   = models_init_cb,
        .models.config_server_cb = config_server_evt_cb
    };
    ERROR_CHECK(mesh_stack_init(&init_params, &m_device_provisioned));
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Server Demo -----\n");

    ERROR_CHECK(app_timer_init());
    hal_leds_init();

#if BUTTON_BOARD
    ERROR_CHECK(hal_buttons_init(button_event_handler));
#endif
    
    hal_pwm_init();
    m_current_on_off = 1;
    m_current_brightness = 50;
    m_current_color_temperature = 50;
    led_event_handler(m_current_on_off, m_current_brightness, m_current_color_temperature);

    nrf_clock_lf_cfg_t lfc_cfg = DEV_BOARD_LF_CLK_CFG;
    ERROR_CHECK(mesh_softdevice_init(lfc_cfg));
    mesh_init();
}

static void start(void)
{
    rtt_input_enable(app_rtt_input_handler, RTT_INPUT_POLL_PERIOD_MS);
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

//    hal_pwm_duty_set(LED_COLOR_WARM_CHANNEL, 100);
//    hal_pwm_duty_set(LED_COLOR_COLD_CHANNEL, 1);
    
    for (;;)
    {
        (void)sd_app_evt_wait();
    }
}
