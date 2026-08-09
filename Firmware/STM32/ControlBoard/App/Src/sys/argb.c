#include "sys/argb.h"

#include <stdbool.h>

#include "sys/gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

typedef enum {
    ARGB_STATE_IDLE = 0,
    ARGB_STATE_UPDATING,
} ARGB_STATE;

typedef enum {
    DMA_HALF_COMPLETE,
    DMA_TRANSFER_COMPLETE
} DMA_EVENT;

typedef struct {
    ARGB_Strip *strip_list;
    uint8_t max_brightness;
    ARGB_STATE current_state;
    ARGB_Strip *current_strip;
    uint8_t current_led_idx;
    uint8_t queued_led_count;
    uint8_t pwm_buffer[BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER];

    DMA_NodeConfTypeDef node_config;
    DMA_NodeTypeDef node_gpdma1_channel0;
    DMA_QListTypeDef list_gpdma1_channel0;
    DMA_HandleTypeDef handle_gpdma1_channel0;
    TIM_HandleTypeDef htim4;
} argb_t;

static argb_t argb = {
    .max_brightness = ARGB_BRIGHTNESS_MAX,
    .current_state = ARGB_STATE_IDLE,
};

#define LED_PULSE_HI 208
#define LED_PULSE_LO 104

/**
  * @brief TIM_Base MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htim: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM4_CLK_ENABLE();

        /* Enable DMA channel. */
        HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

        /* GPDMA1_REQUEST_TIM1_CH1 Init */
        argb.node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
        argb.node_config.Init.Request = GPDMA1_REQUEST_TIM4_CH1;
        argb.node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        argb.node_config.Init.Direction = DMA_MEMORY_TO_PERIPH;
        argb.node_config.Init.SrcInc = DMA_SINC_INCREMENTED;
        argb.node_config.Init.DestInc = DMA_DINC_FIXED;
        argb.node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        argb.node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
        argb.node_config.Init.SrcBurstLength = 1;
        argb.node_config.Init.DestBurstLength = 1;
        argb.node_config.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
        argb.node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        argb.node_config.Init.Mode = DMA_NORMAL;
        argb.node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
        argb.node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
        argb.node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;

        if (HAL_DMAEx_List_BuildNode(&argb.node_config, &argb.node_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_InsertNode(&argb.list_gpdma1_channel0, NULL, &argb.node_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_SetCircularMode(&argb.list_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        argb.handle_gpdma1_channel0.Instance = GPDMA1_Channel0;
        argb.handle_gpdma1_channel0.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        argb.handle_gpdma1_channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
        argb.handle_gpdma1_channel0.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
        argb.handle_gpdma1_channel0.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        argb.handle_gpdma1_channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
        if (HAL_DMAEx_List_Init(&argb.handle_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_LinkQ(&argb.handle_gpdma1_channel0, &argb.list_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(htim, hdma[TIM_DMA_ID_CC1], argb.handle_gpdma1_channel0);

        if (HAL_DMA_ConfigChannelAttributes(&argb.handle_gpdma1_channel0, DMA_CHANNEL_NPRIV) != HAL_OK) {
            Error_Handler();
        }
    }
}

/**
  * @brief TIM_Base MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htim_base: TIM_Base handle pointer
  * @retval None
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM4) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM4_CLK_DISABLE();

        /* Disable DMA. */
        HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_CC1]);
    }
}

void ARGB_Init(void) {
    /* Initialize TIM4_CH1 for ARGB. */
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    argb.htim4.Instance = TIM4;
    argb.htim4.Init.Prescaler = 0;
    argb.htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    argb.htim4.Init.Period = 311; // period = 312 ticks = 1.248 µs, i.e. 801.28 kHz
    argb.htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    argb.htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&argb.htim4) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&argb.htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&argb.htim4) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&argb.htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&argb.htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    GPIO_InitTypeDef gpio_init = {0};

    /* Configure output pin, and assign to TIM4. */
    gpio_init.Pin = ARGB_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(ARGB_Port, &gpio_init);
}

void ARGB_Set(ARGB_Strip *strip, const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b) {
    if (strip == NULL || idx >= strip->count) {
        return;
    }

    if ((strip->leds[idx].r == r) &&
        (strip->leds[idx].g == g) &&
        (strip->leds[idx].b == b)) {
        return;
    }

    strip->leds[idx].r = r;
    strip->leds[idx].g = g;
    strip->leds[idx].b = b;
    strip->dirty = true;
}

void ARGB_Fill(ARGB_Strip *strip, const uint8_t r, const uint8_t g, const uint8_t b) {
    if (strip == NULL || strip->leds == NULL) {
        return;
    }

    for (uint8_t idx = 0; idx < strip->count; idx++) {
        if ((strip->leds[idx].r != r) ||
            (strip->leds[idx].g != g) ||
            (strip->leds[idx].b != b)) {
            strip->dirty = true;
        }

        strip->leds[idx].r = r;
        strip->leds[idx].g = g;
        strip->leds[idx].b = b;
    }
}

void ARGB_Set_Brightness(const uint8_t max_brightness) {
    if (argb.max_brightness == max_brightness) {
        return;
    }

    argb.max_brightness = max_brightness;

    for (ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        strip->dirty = true;
    }
}

static void populate_pwm_data(const ARGB_Strip *strip, const uint8_t led_idx, const uint8_t pwm_offset) {
    uint8_t colour_order = strip->colour_order;
    uint8_t buffer_idx = pwm_offset;

    for (uint8_t c = 0; c < 3; c++) {
        const uint8_t colour = colour_order & 0x03;
        colour_order >>= 2;

        uint8_t colour_val;
        switch (colour) {
            default:
            case COLOUR_RED:
                colour_val = strip->leds[led_idx].r;
                break;
            case COLOUR_GREEN:
                colour_val = strip->leds[led_idx].g;
                break;
            case COLOUR_BLUE:
                colour_val = strip->leds[led_idx].b;
                break;
        }

        if (strip->max_brightness != ARGB_BRIGHTNESS_MAX || argb.max_brightness != ARGB_BRIGHTNESS_MAX) {
            colour_val = (uint8_t) (((uint32_t) colour_val * strip->max_brightness * argb.max_brightness) /
                                    (ARGB_BRIGHTNESS_MAX * ARGB_BRIGHTNESS_MAX));
        }

        for (int8_t bit = 7; bit >= 0; bit--) {
            if (colour_val & (1 << bit)) {
                argb.pwm_buffer[buffer_idx] = LED_PULSE_HI;
            } else {
                argb.pwm_buffer[buffer_idx] = LED_PULSE_LO;
            }

            buffer_idx++;
        }
    }
}

static void clear_pwm_data(const uint8_t pwm_offset) {
    for (uint8_t idx = 0; idx < BITS_PER_LED; idx++) {
        argb.pwm_buffer[pwm_offset + idx] = 0;
    }
}

static bool load_next_led(const uint8_t pwm_offset) {
    while (argb.current_strip != NULL) {
        if (argb.current_led_idx < argb.current_strip->count) {
            populate_pwm_data(argb.current_strip, argb.current_led_idx++, pwm_offset);
            argb.queued_led_count++;
            return true;
        }

        argb.current_strip = argb.current_strip->next;
        argb.current_led_idx = 0;
    }

    clear_pwm_data(pwm_offset);
    return false;
}

static void handle_transfer_event(const DMA_EVENT event) {
    const size_t offset = (event == DMA_HALF_COMPLETE) ? 0 : BITS_PER_LED;

    if (argb.queued_led_count > 0) {
        argb.queued_led_count--;
    }

    load_next_led(offset);

    if (argb.queued_led_count == 0) {
        __HAL_TIM_SET_COMPARE(&argb.htim4, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Stop_DMA(&argb.htim4, TIM_CHANNEL_1);
        argb.current_state = ARGB_STATE_IDLE;
    }
}

void GPDMA1_Channel0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&argb.handle_gpdma1_channel0);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    handle_transfer_event(DMA_TRANSFER_COMPLETE);
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
    handle_transfer_event(DMA_HALF_COMPLETE);
}

void ARGB_Add_Strip(ARGB_Strip *strip) {
    if (strip == NULL || strip->leds == NULL || strip->count == 0) {
        return;
    }

    strip->next = NULL;
    strip->dirty = true;

    if (argb.strip_list == NULL) {
        argb.strip_list = strip;
    } else {
        ARGB_Strip *tail = argb.strip_list;
        while (tail->next != NULL) {
            tail = tail->next;
        }

        tail->next = strip;
    }
}

static bool strips_are_dirty(void) {
    for (const ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        if (strip->dirty) {
            return true;
        }
    }

    return false;
}

static void clear_strip_dirty_flags(void) {
    for (ARGB_Strip *strip = argb.strip_list; strip != NULL; strip = strip->next) {
        strip->dirty = false;
    }
}

void ARGB_Service(void) {
    if (argb.current_state != ARGB_STATE_IDLE || argb.strip_list == NULL) {
        return;
    }

    if (!strips_are_dirty()) {
        return;
    }

    clear_strip_dirty_flags();

    argb.current_strip = argb.strip_list;
    argb.current_led_idx = 0;
    argb.queued_led_count = 0;
    argb.current_state = ARGB_STATE_UPDATING;

    load_next_led(0);
    load_next_led(BITS_PER_LED);

    if (argb.queued_led_count == 0) {
        argb.current_state = ARGB_STATE_IDLE;
        return;
    }

    __HAL_TIM_SET_COMPARE(&argb.htim4, TIM_CHANNEL_1, 0);

    // Populate the first LED with the data.
    if (HAL_TIM_PWM_Start_DMA(&argb.htim4, TIM_CHANNEL_1, (uint32_t *) &argb.pwm_buffer[0],
                              BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER) != HAL_OK) {
        Error_Handler();
    }
}
