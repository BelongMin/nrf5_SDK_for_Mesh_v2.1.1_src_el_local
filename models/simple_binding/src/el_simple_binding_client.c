/**************************************************************************
  * @file       : el_simple_binding_client.c
  * @brief      : create a new model client for control the binding of light
  * @author     : Belong.Lin 
  * @copyright  : ALl rights reserved by Extra Light
  * @version    : 0.1
  * @note       : none
  * @history    : create @20180611
***************************************************************************/
#include "el_simple_binding_client.h"

#include <stdint.h>
#include <stddef.h>

#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "device_state_manager.h"
#include "nrf_mesh.h"
#include "nrf_mesh_assert.h"
#include "log.h"

/*****************************************************************************
 * Static variables
 *****************************************************************************/

/** Keeps a single global TID for all transfers. */
static uint8_t m_tid;

/*****************************************************************************
 * Static functions
 *****************************************************************************/

static void reliable_status_cb(access_model_handle_t model_handle, void * p_args, access_reliable_status_t status)
{
    el_simple_binding_client_t * p_client = p_args;
    NRF_MESH_ASSERT(p_client->status_cb != NULL);

    p_client->state.reliable_transfer_active = false;
    switch (status)
    {
        case ACCESS_RELIABLE_TRANSFER_SUCCESS:
            /* Ignore */
            break;
        case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
            p_client->status_cb(p_client, EL_SIMPLE_BINDING_STATUS_ERROR_NO_REPLY, NRF_MESH_ADDR_UNASSIGNED);
            break;
        case ACCESS_RELIABLE_TRANSFER_CANCELLED:
            p_client->status_cb(p_client, EL_SIMPLE_BINDING_STATUS_CANCELLED, NRF_MESH_ADDR_UNASSIGNED);
            break;
        default:
            /* Should not be possible. */
            NRF_MESH_ASSERT(false);
            break;
    }
}

/** Returns @c true if the message received was from the address corresponding to the clients publish address. */
static bool is_valid_source(const el_simple_binding_client_t * p_client,
                            const access_message_rx_t * p_message)
{
    /* Check the originator of the status. */
    dsm_handle_t publish_handle;
    nrf_mesh_address_t publish_address;
    if (access_model_publish_address_get(p_client->model_handle, &publish_handle) != NRF_SUCCESS ||
        publish_handle == DSM_HANDLE_INVALID ||
        dsm_address_get(publish_handle, &publish_address) != NRF_SUCCESS ||
        publish_address.value != p_message->meta_data.src.value)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static uint32_t send_reliable_message(const el_simple_binding_client_t * p_client, el_simple_binding_opcode_t opcode, const uint8_t * p_data, uint16_t length)
{
    access_reliable_t reliable;
    reliable.model_handle = p_client->model_handle;
    reliable.message.p_buffer = p_data;
    reliable.message.length = length;
    reliable.message.opcode.opcode = opcode;
    reliable.message.opcode.company_id = EL_SIMPLE_BINDING_COMPANY_ID;
    reliable.message.force_segmented = false;
    reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
    reliable.reply_opcode.opcode = EL_SIMPLE_BINDING_OPCODE_STATUS;
    reliable.reply_opcode.company_id = EL_SIMPLE_BINDING_COMPANY_ID;
    reliable.timeout = ACCESS_RELIABLE_TIMEOUT_MIN;
    reliable.status_cb = reliable_status_cb;

    return access_model_reliable_publish(&reliable);
}

/*****************************************************************************
 * Opcode handler callback(s)
 *****************************************************************************/
static void handle_status_cb(access_model_handle_t handle, const access_message_rx_t * p_message, void * p_args)
{
    el_simple_binding_client_t * p_client = p_args;
    NRF_MESH_ASSERT(p_client->status_cb != NULL);

    if (!is_valid_source(p_client, p_message))
    {
        return;
    }

    el_simple_binding_msg_status_t * p_status = (el_simple_binding_msg_status_t *)p_message->p_data;

    // if(p_status->present_binding >= EL_SIMPLE_BINDING_VALUE_MIN && p_status->present_binding <= EL_SIMPLE_BINDING_VALUE_MAX)
    // {
    //     el_simple_binding_status_t binding_status = EL_SIMPLE_BINDING_STATUS_NORMAL;
    //     p_client->status_cb(p_client, binding_status, p_message->meta_data.src.value);
    // }
    if(p_status->present_binding_data.binding_type != 0x00)
    {
        el_simple_binding_status_t binding_status = EL_SIMPLE_BINDING_STATUS_NORMAL;
        p_client->status_cb(p_client, binding_status, p_message->meta_data.src.value);
    }
}

static const access_opcode_handler_t m_opcode_handlers[] = 
{
     {{EL_SIMPLE_BINDING_OPCODE_STATUS, EL_SIMPLE_BINDING_COMPANY_ID}, handle_status_cb}
};

static void handle_publish_timeout(access_model_handle_t handle, void * p_args)
{
    el_simple_binding_client_t * p_client = p_args;

    if (p_client->timeout_cb != NULL)
    {
        p_client->timeout_cb(handle, p_args);
    }
}

/*****************************************************************************
 * Public API
 *****************************************************************************/
uint32_t el_simple_binding_client_init(el_simple_binding_client_t * p_client, uint16_t element_index)
{
    if (p_client == NULL ||
        p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }

    access_model_add_params_t init_params;
    init_params.model_id.model_id = EL_SIMPLE_BINDING_MODEL_CLIENT_ID;
    init_params.model_id.company_id = EL_SIMPLE_BINDING_COMPANY_ID;
    init_params.element_index = element_index;
    init_params.p_opcode_handlers = &m_opcode_handlers[0];
    init_params.opcode_count = sizeof(m_opcode_handlers) / sizeof(m_opcode_handlers[0]);
    init_params.p_args = p_client;
    init_params.publish_timeout_cb = handle_publish_timeout;
    return access_model_add(&init_params, &p_client->model_handle);
}

uint32_t el_simple_binding_client_set(el_simple_binding_client_t * p_client, el_simple_binding_data_t binding_data)
{
     if (p_client == NULL || p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }
    else if (p_client->state.reliable_transfer_active)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    p_client->state.data.binding_data = binding_data;
    p_client->state.data.tid = m_tid++;

    uint32_t status = send_reliable_message(p_client,
                                            EL_SIMPLE_BINDING_OPCODE_SET,
                                            (const uint8_t *)&p_client->state.data,
                                            sizeof(el_simple_binding_msg_set_t));
    if (status == NRF_SUCCESS)
    {
        p_client->state.reliable_transfer_active = true;
    }
    return status;
}

uint32_t el_simple_binding_client_set_unreliable(el_simple_binding_client_t * p_client, el_simple_binding_data_t binding_data, uint8_t repeats)
{
    el_simple_binding_msg_set_unreliable_t set_unreliable;
    set_unreliable.binding_data = binding_data;
    set_unreliable.tid = m_tid++;

    access_message_tx_t message;
    message.opcode.opcode = EL_SIMPLE_BINDING_OPCODE_SET_UNRELIABLE;
    message.opcode.company_id = EL_SIMPLE_BINDING_COMPANY_ID;
    message.p_buffer = (const uint8_t*) &set_unreliable;
    message.length = sizeof(set_unreliable);
    message.force_segmented = false;
    message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;

    uint32_t status = NRF_SUCCESS;
    for (uint8_t i = 0; i < repeats; ++i)
    {
        status = access_model_publish(p_client->model_handle, &message);
        if (status != NRF_SUCCESS)
        {
            break;
        }
    }
    return status;
}

uint32_t el_simple_binding_client_get(el_simple_binding_client_t * p_client)
{
    if (p_client == NULL || p_client->status_cb == NULL)
    {
        return NRF_ERROR_NULL;
    }
    else if (p_client->state.reliable_transfer_active)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    uint32_t status = send_reliable_message(p_client,
                                            EL_SIMPLE_BINDING_OPCODE_GET,
                                            NULL,
                                            0);
    if (status == NRF_SUCCESS)
    {
        p_client->state.reliable_transfer_active = true;
    }
    return status;
}

/**
 * Cancel any ongoing reliable message transfer.
 *
 * @param[in] p_client Pointer to the client instance structure.
 */
void el_simple_binding_client_pending_msg_cancel(el_simple_binding_client_t * p_client)
{
    (void)access_model_reliable_cancel(p_client->model_handle);
}
