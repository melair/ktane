/**
  ******************************************************************************
  * @file           : mcu_init.c
  * @brief          : MCU, clock, GPIO, and cache initialization.
  ******************************************************************************
  */
#include "mcu_init.h"
#include "main.h"
#include "stm32h5xx_it.h"

/* MPU Configuration */
void MPU_Init(void) {
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

    /* Disables the MPU */
    HAL_MPU_Disable();

    /* Initializes and configures the Region 0 and the memory to be protected */
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x08FFF000;
    MPU_InitStruct.LimitAddress = 0x08FFFFFF;
    MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
    MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Flash high-cycle data must not be accessed through the instruction cache. */
    MPU_InitStruct.Number = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress = FLASH_EDATA_BASE;
    MPU_InitStruct.LimitAddress = FLASH_EDATA_BASE + FLASH_EDATA_SIZE - 1;
    MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RW;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Initializes and configures the Attribute 0 and the memory to be protected */
    MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
    MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

    HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);

    /* Enables the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Init(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    /* Initializes the RCC Oscillators according to the specified parameters. */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS_DIGITAL;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                  | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }

    /* Configure the programming delay */
    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);

    RCC_OscInitTypeDef RCC_HSIOscInitStruct = {0};

    RCC_HSIOscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_HSIOscInitStruct.HSI48State = RCC_HSI48_ON;
    if (HAL_RCC_OscConfig(&RCC_HSIOscInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief Peripheral Clock Configuration
  * @retval None
  */
void PeripheralClock_Init(void) {
    /* Initialise PLL2 primarily for SDMMC, ADC and FDCAN, 80MHz on PLL2Q, 25MHz on PLL2R and 200MHz on PLL2P */
    RCC_PLL2InitTypeDef pll2InitStruct = {
        .PLL2Source = RCC_PLL2_SOURCE_HSE,
        .PLL2M = 10,
        .PLL2N = 160,
        .PLL2P = 2,
        .PLL2Q = 5,
        .PLL2R = 16,
        .PLL2RGE = RCC_PLL2_VCIRANGE_1,
        .PLL2VCOSEL = RCC_PLL2_VCORANGE_WIDE,
        .PLL2FRACN = 0,
        .PLL2ClockOut = RCC_PLL2_DIVQ | RCC_PLL2_DIVR,
    };

    if (HAL_RCCEx_EnablePLL2(&pll2InitStruct) != HAL_OK) {
        Error_Handler();
    }

    /* Initialise PLL3 primarily for I2S, 12.287979MHz on PLL3P */
    RCC_PLL3InitTypeDef pll3InitStruct = {
        .PLL3Source = RCC_PLL3_SOURCE_HSE,
        .PLL3M = 5,
        .PLL3N = 39,
        .PLL3P = 16,
        .PLL3Q = 2,
        .PLL3R = 2,
        .PLL3RGE = RCC_PLL3_VCIRANGE_0,
        .PLL3VCOSEL = RCC_PLL3_VCORANGE_MEDIUM,
        .PLL3FRACN = 2634,
        .PLL3ClockOut = RCC_PLL3_DIVP,
    };

    if (HAL_RCCEx_EnablePLL3(&pll3InitStruct) != HAL_OK) {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void GPIO_Init(void) {
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
void ICACHE_Init(void) {
    /* Enable instruction cache in 1-way (direct mapped cache) */
    if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_ICACHE_Enable() != HAL_OK) {
        Error_Handler();
    }
}
