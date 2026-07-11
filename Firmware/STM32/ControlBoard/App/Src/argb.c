#include "argb.h"

#include <stdbool.h>
#include <stdlib.h>

#include "gpio.h"
#include "stm32h5xx_hal.h"
#include "stm32h5xx_it.h"

typedef struct {
    uint8_t r, g, b;
} ARGB_LED;

typedef enum {
    ARGB_STATE_IDLE = 0,
    ARGB_STATE_UPDATING,
} ARGB_STATE;

typedef enum {
    DMA_HALF_COMPLETE,
    DMA_TRANSFER_COMPLETE
} DMA_EVENT;

struct ARGB_Strip {
    uint8_t count;
    uint8_t colour_order;
    ARGB_LED *leds;
    ARGB_Strip *next;
};

ARGB_Strip *ARGB_Indicator_Strip = NULL;

static ARGB_Strip *strip_list = NULL;
static ARGB_STATE current_state = ARGB_STATE_IDLE;
static ARGB_Strip *current_strip = NULL;
static uint8_t current_led_idx = 0;
static uint8_t queued_led_count = 0;
static uint8_t pwm_buffer[BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER] = {0};

static DMA_NodeConfTypeDef node_config = {0};
static DMA_NodeTypeDef node_gpdma1_channel0 = {0};
static DMA_QListTypeDef list_gpdma1_channel0 = {0};
static DMA_HandleTypeDef handle_gpdma1_channel0 = {0};
static TIM_HandleTypeDef htim4 = {0};

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
        node_config.NodeType = DMA_GPDMA_LINEAR_NODE;
        node_config.Init.Request = GPDMA1_REQUEST_TIM4_CH1;
        node_config.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
        node_config.Init.Direction = DMA_MEMORY_TO_PERIPH;
        node_config.Init.SrcInc = DMA_SINC_INCREMENTED;
        node_config.Init.DestInc = DMA_DINC_FIXED;
        node_config.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
        node_config.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
        node_config.Init.SrcBurstLength = 1;
        node_config.Init.DestBurstLength = 1;
        node_config.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
        node_config.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        node_config.Init.Mode = DMA_NORMAL;
        node_config.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
        node_config.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
        node_config.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;

        if (HAL_DMAEx_List_BuildNode(&node_config, &node_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_InsertNode(&list_gpdma1_channel0, NULL, &node_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_SetCircularMode(&list_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        handle_gpdma1_channel0.Instance = GPDMA1_Channel0;
        handle_gpdma1_channel0.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        handle_gpdma1_channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
        handle_gpdma1_channel0.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
        handle_gpdma1_channel0.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
        handle_gpdma1_channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;
        if (HAL_DMAEx_List_Init(&handle_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_DMAEx_List_LinkQ(&handle_gpdma1_channel0, &list_gpdma1_channel0) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(htim, hdma[TIM_DMA_ID_CC1], handle_gpdma1_channel0);

        if (HAL_DMA_ConfigChannelAttributes(&handle_gpdma1_channel0, DMA_CHANNEL_NPRIV) != HAL_OK) {
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
    /* Add module indicator. */
    ARGB_Indicator_Strip = ARGB_Add_Strip(COLOUR_ORDER_GRB, INDICATOR_COUNT);

    /* Initialize TIM4_CH1 for ARGB. */
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 311; // period = 312 ticks = 1.248 µs, i.e. 801.28 kHz
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    GPIO_InitTypeDef gpio_init = {0};

    /* Configure output pin, and assign to TIM4. */
    gpio_init.Pin = ARGB_Pin;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(ARGB_GPIO_Port, &gpio_init);
}

void ARGB_Set(const ARGB_Strip *strip, const uint8_t idx, const uint8_t r, const uint8_t g, const uint8_t b) {
    if (strip == NULL || idx >= strip->count) {
        return;
    }

    strip->leds[idx].r = r;
    strip->leds[idx].g = g;
    strip->leds[idx].b = b;
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

        for (int8_t bit = 7; bit >= 0; bit--) {
            if (colour_val & (1 << bit)) {
                pwm_buffer[buffer_idx] = LED_PULSE_HI;
            } else {
                pwm_buffer[buffer_idx] = LED_PULSE_LO;
            }

            buffer_idx++;
        }
    }
}

static void clear_pwm_data(const uint8_t pwm_offset) {
    for (uint8_t idx = 0; idx < BITS_PER_LED; idx++) {
        pwm_buffer[pwm_offset + idx] = 0;
    }
}

static bool load_next_led(const uint8_t pwm_offset) {
    while (current_strip != NULL) {
        if (current_led_idx < current_strip->count) {
            populate_pwm_data(current_strip, current_led_idx++, pwm_offset);
            queued_led_count++;
            return true;
        }

        current_strip = current_strip->next;
        current_led_idx = 0;
    }

    clear_pwm_data(pwm_offset);
    return false;
}

static void handle_transfer_event(const DMA_EVENT event) {
    const size_t offset = (event == DMA_HALF_COMPLETE) ? 0 : BITS_PER_LED;

    if (queued_led_count > 0) {
        queued_led_count--;
    }

    load_next_led(offset);

    if (queued_led_count == 0) {
        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_1);
        current_state = ARGB_STATE_IDLE;
    }
}

void GPDMA1_Channel0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&handle_gpdma1_channel0);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    handle_transfer_event(DMA_TRANSFER_COMPLETE);
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
    handle_transfer_event(DMA_HALF_COMPLETE);
}

ARGB_Strip *ARGB_Add_Strip(const uint8_t colour_order, const uint8_t led_count) {
    if (led_count == 0) {
        return NULL;
    }

    ARGB_Strip *new_strip = malloc(sizeof(ARGB_Strip));
    if (new_strip == NULL) {
        return NULL;
    }

    new_strip->count = led_count;
    new_strip->colour_order = colour_order;
    new_strip->next = NULL;
    new_strip->leds = calloc(led_count, sizeof(ARGB_LED));
    if (new_strip->leds == NULL) {
        free(new_strip);
        return NULL;
    }

    if (strip_list == NULL) {
        strip_list = new_strip;
    } else {
        ARGB_Strip *tail = strip_list;
        while (tail->next != NULL) {
            tail = tail->next;
        }

        tail->next = new_strip;
    }

    return new_strip;
}

void ARGB_Update(void) {
    if (current_state != ARGB_STATE_IDLE || strip_list == NULL) {
        return;
    }

    current_strip = strip_list;
    current_led_idx = 0;
    queued_led_count = 0;
    current_state = ARGB_STATE_UPDATING;

    load_next_led(0);
    load_next_led(BITS_PER_LED);

    if (queued_led_count == 0) {
        current_state = ARGB_STATE_IDLE;
        return;
    }

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 0);

    // Populate the first LED with the data.
    if (HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_1, (uint32_t *) &pwm_buffer[0],
                              BITS_PER_LED * LEDS_IN_TRANSPORT_BUFFER) != HAL_OK) {
        Error_Handler();
    }
}
