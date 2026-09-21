#include <string.h>

#include "main.h"
#include "app_common.h"
#include "ble.h"
#include "ble_evt.h"
#include "ktane_gatt.h"
#include "app_ble.h"

#define CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET      (1U)
#define KTANE_GATT_MAX_RESPONSE_CLIENTS            APP_BLE_MAX_CONNECTIONS
#define KTANE_GATT_INVALID_CONNECTION_HANDLE        (0xFFFFU)

typedef struct {
  uint16_t control_request_char_handle;
  uint16_t control_response_char_handle;
} ktane_gatt_context_t;

typedef struct {
  uint16_t connection_handle;
  uint16_t response_length;
  uint8_t response[KTANE_GATT_CONTROL_RESPONSE_MAX_LEN];
} control_response_t;

static ktane_gatt_context_t ktane_gatt_context;
static control_response_t control_responses[KTANE_GATT_MAX_RESPONSE_CLIENTS];

static uint8_t battery_level;
static uint8_t game_state;
static uint8_t game_mode;
static uint8_t game_timer[sizeof(uint32_t)];
static uint8_t game_strikes[2];

static ble_gatt_val_buffer_def_t battery_level_buffer = {
  .val_len = sizeof(battery_level),
  .buffer_len = sizeof(battery_level),
  .buffer_p = &battery_level,
};
static ble_gatt_val_buffer_def_t game_state_buffer = {
  .val_len = sizeof(game_state),
  .buffer_len = sizeof(game_state),
  .buffer_p = &game_state,
};
static ble_gatt_val_buffer_def_t game_mode_buffer = {
  .val_len = sizeof(game_mode),
  .buffer_len = sizeof(game_mode),
  .buffer_p = &game_mode,
};
static ble_gatt_val_buffer_def_t game_timer_buffer = {
  .val_len = sizeof(game_timer),
  .buffer_len = sizeof(game_timer),
  .buffer_p = game_timer,
};
static ble_gatt_val_buffer_def_t game_strikes_buffer = {
  .val_len = sizeof(game_strikes),
  .buffer_len = sizeof(game_strikes),
  .buffer_p = game_strikes,
};

BLE_GATT_SRV_CCCD_DECLARE(control_response, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(game_state, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(game_event_stream, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(game_mode, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(game_timer, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(game_strikes, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_AUTHEN_WRITE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);
BLE_GATT_SRV_CCCD_DECLARE(battery_level, CFG_BLE_NUM_RADIO_TASKS,
                          BLE_GATT_SRV_PERM_NONE,
                          BLE_GATT_SRV_OP_MODIFIED_EVT_ENABLE_FLAG);

static const ble_gatt_srv_def_t information_service = {
  .type = BLE_GATT_SRV_PRIMARY_SRV_TYPE,
  .uuid = BLE_UUID_INIT_128(KTANE_INFORMATION_SERVICE_UUID_128),
};

static const ble_gatt_chr_def_t control_chars[] = {
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_WRITE,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_WRITE,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_CONTROL_COMMAND_REQUEST_UUID_128),
    .val_buffer_p = NULL,
  },
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_INDICATE,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_CONTROL_COMMAND_RESPONSE_UUID_128),
    .descrs = {
      .descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(control_response),
      .descr_count = 1U,
    },
    .val_buffer_p = NULL,
  },
};
static const ble_gatt_srv_def_t control_service = {
  .type = BLE_GATT_SRV_PRIMARY_SRV_TYPE,
  .uuid = BLE_UUID_INIT_128(KTANE_CONTROL_SERVICE_UUID_128),
  .chrs = {
    .chrs_p = (ble_gatt_chr_def_t *) control_chars,
    .chr_count = sizeof(control_chars) / sizeof(control_chars[0]),
  },
};

static const ble_gatt_chr_def_t game_chars[] = {
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_INDICATE,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_GAME_EVENT_STREAM_UUID_128),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(game_event_stream), .descr_count = 1U},
    .val_buffer_p = NULL,
  },
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_GAME_STATE_UUID_128),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(game_state), .descr_count = 1U},
    .val_buffer_p = &game_state_buffer,
  },
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_GAME_MODE_UUID_128),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(game_mode), .descr_count = 1U},
    .val_buffer_p = &game_mode_buffer,
  },
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_GAME_TIMER_UUID_128),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(game_timer), .descr_count = 1U},
    .val_buffer_p = &game_timer_buffer,
  },
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY,
    .permissions = BLE_GATT_SRV_PERM_AUTHEN_READ,
    .min_key_size = BLE_GATT_SRV_MIN_ENCRY_KEY_SIZE,
    .uuid = BLE_UUID_INIT_128(KTANE_GAME_STRIKES_UUID_128),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(game_strikes), .descr_count = 1U},
    .val_buffer_p = &game_strikes_buffer,
  },
};
static const ble_gatt_srv_def_t game_service = {
  .type = BLE_GATT_SRV_PRIMARY_SRV_TYPE,
  .uuid = BLE_UUID_INIT_128(KTANE_GAME_SERVICE_UUID_128),
  .chrs = {
    .chrs_p = (ble_gatt_chr_def_t *) game_chars,
    .chr_count = sizeof(game_chars) / sizeof(game_chars[0]),
  },
};

static const ble_gatt_chr_def_t battery_chars[] = {
  {
    .properties = BLE_GATT_SRV_CHAR_PROP_READ | BLE_GATT_SRV_CHAR_PROP_NOTIFY,
    .permissions = BLE_GATT_SRV_PERM_NONE,
    .uuid = BLE_UUID_INIT_16(0x2A19),
    .descrs = {.descrs_p = &BLE_GATT_SRV_CCCD_DEF_NAME(battery_level), .descr_count = 1U},
    .val_buffer_p = &battery_level_buffer,
  },
};
static const ble_gatt_srv_def_t battery_service = {
  .type = BLE_GATT_SRV_PRIMARY_SRV_TYPE,
  .uuid = BLE_UUID_INIT_16(0x180F),
  .chrs = {
    .chrs_p = (ble_gatt_chr_def_t *) battery_chars,
    .chr_count = sizeof(battery_chars) / sizeof(battery_chars[0]),
  },
};

static control_response_t *find_control_response(uint16_t connection_handle) {
  control_response_t *available = NULL;

  for (uint32_t index = 0U; index < KTANE_GATT_MAX_RESPONSE_CLIENTS; index++) {
    if (control_responses[index].connection_handle == connection_handle) {
      return &control_responses[index];
    }
    if ((available == NULL) &&
        (control_responses[index].connection_handle == KTANE_GATT_INVALID_CONNECTION_HANDLE)) {
      available = &control_responses[index];
    }
  }

  return available;
}

static BLEEVT_EvtAckStatus_t ktane_gatt_event_handler(aci_blecore_event *event) {
  if (event->ecode == ACI_GATT_SRV_WRITE_VSEVT_CODE) {
    aci_gatt_srv_write_event_rp0 *write = (aci_gatt_srv_write_event_rp0 *) event->data;
    if (write->Attribute_Handle == ktane_gatt_context.control_request_char_handle +
        CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET) {
      KTANE_GATT_OnCommandRequest(write->Connection_Handle, write->Data, write->Data_Length);
      if (write->Resp_Needed != 0U) {
        (void) aci_gatt_srv_resp(write->Connection_Handle, write->CID, write->Attribute_Handle,
                                 BLE_ATT_ERR_NONE, 0U, NULL);
      }
      return BLEEVT_Ack;
    }
  } else if (event->ecode == ACI_GATT_SRV_READ_VSEVT_CODE) {
    aci_gatt_srv_read_event_rp0 *read = (aci_gatt_srv_read_event_rp0 *) event->data;
    if (read->Attribute_Handle == ktane_gatt_context.control_response_char_handle +
        CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET) {
      control_response_t *response = find_control_response(read->Connection_Handle);
      const uint8_t *data = (response == NULL) ? NULL : response->response;
      uint16_t length = (response == NULL) ? 0U : response->response_length;
      (void) aci_gatt_srv_resp(read->Connection_Handle, read->CID, read->Attribute_Handle,
                               BLE_ATT_ERR_NONE, length, (uint8_t *) data);
      return BLEEVT_Ack;
    }
  }

  return BLEEVT_NoAck;
}

__WEAK void KTANE_GATT_OnCommandRequest(uint16_t connection_handle,
                                        const uint8_t *command,
                                        uint16_t command_length) {
  UNUSED(connection_handle);
  UNUSED(command);
  UNUSED(command_length);
}

void KTANE_GATT_Init(void) {
  static const ble_gatt_srv_def_t *const services[] = {
    &information_service, &control_service, &game_service, &battery_service,
  };

  for (uint32_t index = 0U; index < KTANE_GATT_MAX_RESPONSE_CLIENTS; index++) {
    control_responses[index].connection_handle = KTANE_GATT_INVALID_CONNECTION_HANDLE;
  }

  if (BLEEVT_RegisterGattEvtHandler(ktane_gatt_event_handler) != 0) {
    Error_Handler();
  }
  for (uint32_t index = 0U; index < (sizeof(services) / sizeof(services[0])); index++) {
    if (aci_gatt_srv_add_service((ble_gatt_srv_def_t *) services[index]) != BLE_STATUS_SUCCESS) {
      Error_Handler();
    }
  }

  ktane_gatt_context.control_request_char_handle =
      aci_gatt_srv_get_char_decl_handle((ble_gatt_chr_def_t *) &control_chars[0]);
  ktane_gatt_context.control_response_char_handle =
      aci_gatt_srv_get_char_decl_handle((ble_gatt_chr_def_t *) &control_chars[1]);
}

void KTANE_GATT_OnDisconnected(uint16_t connection_handle) {
  control_response_t *response = find_control_response(connection_handle);
  if (response != NULL) {
    response->connection_handle = KTANE_GATT_INVALID_CONNECTION_HANDLE;
    response->response_length = 0U;
  }
}

tBleStatus KTANE_GATT_SendControlResponse(uint16_t connection_handle,
                                          const uint8_t *response,
                                          uint16_t response_length) {
  control_response_t *slot;

  if ((response_length > KTANE_GATT_CONTROL_RESPONSE_MAX_LEN) ||
      ((response_length != 0U) && (response == NULL))) {
    return BLE_STATUS_INVALID_PARAMS;
  }
  slot = find_control_response(connection_handle);
  if (slot == NULL) {
    return BLE_STATUS_INSUFFICIENT_RESOURCES;
  }

  slot->connection_handle = connection_handle;
  slot->response_length = response_length;
  if (response_length != 0U) {
    memcpy(slot->response, response, response_length);
  }

  return aci_gatt_srv_notify(connection_handle, BLE_GATT_UNENHANCED_ATT_L2CAP_CID,
                             ktane_gatt_context.control_response_char_handle +
                             CHARACTERISTIC_VALUE_ATTRIBUTE_OFFSET,
                             GATT_INDICATION, response_length, slot->response);
}
