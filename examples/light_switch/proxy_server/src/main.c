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
#include "el_simple_brightness_server.h" //Add by belong.lin
#include "el_simple_brightness_common.h" //Add by belong.lin
#include "light_switch_example_common.h"
#include "mesh_app_utils.h"
#include "net_state.h"
#include "rtt_input.h"
#include "mesh_stack.h"
#include "mesh_provisionee.h"
#include "nrf_mesh_configure.h"
#include "nrf_mesh_config_examples.h"
#include "mesh_adv.h"

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "ble_conn_params.h"
#include "sdk_config.h"
#include "proxy.h"
#include "app_timer.h"

#define DEVICE_NAME                     "nRF5x Mesh Light"
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(150,  UNIT_1_25_MS)           /**< Minimum acceptable connection interval. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(250,  UNIT_1_25_MS)           /**< Maximum acceptable connection interval. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(100)                        /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called. */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(2000)                       /**< Time between each call to sd_ble_gap_conn_param_update after the first call. */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define RTT_INPUT_POLL_PERIOD_MS (100)
#define LED_PIN_NUMBER (BSP_LED_0)
#define LED_PIN_MASK   (1u << LED_PIN_NUMBER)
#define LED_BLINK_INTERVAL_MS    (200)
#define LED_BLINK_CNT_START      (2)
#define LED_BLINK_CNT_RESET      (3)
#define LED_BLINK_CNT_PROV       (4)

static void gap_params_init(void);
static void conn_params_init(void);

static void on_sd_evt(uint32_t sd_evt, void * p_context)
{
    (void) nrf_mesh_on_sd_evt(sd_evt);
}

NRF_SDH_SOC_OBSERVER(mesh_observer, NRF_SDH_BLE_STACK_OBSERVER_PRIO, on_sd_evt, NULL);

static simple_on_off_server_t m_server;
static el_simple_brightness_server_t m_brightness_server;
static uint8_t m_device_brightness;
static bool                   m_device_provisioned;


static void provisioning_complete_cb(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully provisioned\n");

    /* Restores the application parameters after switching from the Provisioning service to the Proxy  */
    gap_params_init();
    conn_params_init();

    dsm_local_unicast_address_t node_address;
    dsm_local_unicast_addresses_get(&node_address);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Node Address: 0x%04x \n", node_address.address_start);

    hal_led_mask_set(LEDS_MASK, false);
    hal_led_blink_ms(LED_PIN_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_PROV);
}

static bool on_off_server_get_cb(const simple_on_off_server_t * p_server)
{
    return hal_led_pin_get(LED_PIN_NUMBER);
}

static bool on_off_server_set_cb(const simple_on_off_server_t * p_server, bool value)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Got SET command to %u\n", value);
    hal_led_pin_set(LED_PIN_NUMBER, value);
//    hal_led_pin_set(LED_L1, value);
//    hal_led_pin_set(LED_L2, value);
    if(value)
    {
        hal_pwm_duty_set(0, 50);
        hal_pwm_duty_set(1, 1);
    }
    else
    {
        hal_pwm_duty_set(0, 0);
        hal_pwm_duty_set(1, 100);
    }
    return value;
}


static uint8_t el_brightness_get_cb(const el_simple_brightness_server_t * p_server)
{
    return m_device_brightness;
}

static uint8_t el_brightness_set_cb(const el_simple_brightness_server_t * p_server, uint8_t value)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "--->Brightness of device has been set to %u<---\n", value);
    
    
    m_device_brightness = value;
    hal_pwm_duty_set(0, m_device_brightness);
    hal_pwm_duty_set(1, m_device_brightness);
    
    return value;
}

static void node_reset(void)
{
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- Node reset  -----\n");
    hal_led_blink_ms(LED_PIN_MASK, LED_BLINK_INTERVAL_MS, LED_BLINK_CNT_RESET);
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
    switch (button_number)
    {
        /* Pressing SW1 on the Development Kit will result in LED state to toggle and trigger
        the STATUS message to inform client about the state change. This is a demonstration of
        state change publication due to local event. */
        case 0:
        {
            uint8_t value = !hal_led_pin_get(LED_PIN_NUMBER);
            __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "User action \n");
            hal_led_pin_set(LED_PIN_NUMBER, value);
            (void)simple_on_off_server_status_publish(&m_server, value);
            break;
        }

        /* Initiate node reset */
        case 3:
        {
            /* Clear all the states to reset the node. */
            proxy_disable();
            mesh_stack_config_clear();
            node_reset();
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

static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(p_evt->conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
    else if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
        __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Successfully updated connection parameters\n");
    }
}

static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
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

static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}

static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    ble_gap_conn_params_t  gap_conn_params;

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    memset(&cp_init, 0, sizeof(cp_init));
    cp_init.p_conn_params                  = &gap_conn_params;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

static void initialize(void)
{
    __LOG_INIT(LOG_SRC_APP | LOG_SRC_ACCESS | LOG_SRC_BEARER, LOG_LEVEL_INFO, LOG_CALLBACK_DEFAULT);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "----- BLE Mesh Light Switch Proxy Server Demo -----\n");

    ERROR_CHECK(app_timer_init());
    hal_leds_init();

#if BUTTON_BOARD
    ERROR_CHECK(hal_buttons_init(button_event_handler));
#endif

    hal_pwm_init();

    uint32_t err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    uint32_t ram_start = 0;
    /* Set the default configuration (as defined through sdk_config.h). */
    err_code = nrf_sdh_ble_default_cfg_set(MESH_SOFTDEVICE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    gap_params_init();
    conn_params_init();

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
