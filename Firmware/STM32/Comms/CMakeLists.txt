set(COMMS_TARGET Comms)
add_executable(${COMMS_TARGET})

set(COMMS_INCLUDE_DIRS
        App/Inc App/Inc/BLE App/Inc/Debug Drivers/MCU_Support/Inc
        Drivers/HAL/Inc Drivers/HAL/Inc/Legacy
        Drivers/CMSIS_Device/Include Drivers/CMSIS_Core/Core/Include
        Drivers/BLE_Stack/Utilities/trace/adv_trace Drivers/BLE_Stack/Utilities/misc
        Drivers/BLE_Stack/Utilities/sequencer Drivers/BLE_Stack/Utilities/lpm/tiny_lpm
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces Drivers/BLE_Stack/Projects/Common/BLE/Modules
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/RTDebug
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/RADIO_utils/Inc
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Profiles/Inc
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/PKAMGR/Inc
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/NVMDB/Inc
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Flash Drivers/BLE_Stack/Projects/Common/BLE/Startup
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/Common
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/AES
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CBC
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/CMAC
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/Common
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib/Inc/AES/ECB
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/evt_handler/inc
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/queued_writes/inc
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/stack/include
)
list(TRANSFORM COMMS_INCLUDE_DIRS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

set(COMMS_APP_SOURCES
        App/Src/main.c App/Src/app_entry.c App/Src/Debug/usart_if.c
        App/Src/BLE/blenvm.c App/Src/BLE/bleplat.c App/Src/BLE/bleplat_cntr.c
        App/Src/BLE/app_ble.c App/Src/BLE/ktane_gatt.c
)
set(COMMS_MCU_SUPPORT_SOURCES
        Drivers/MCU_Support/Src/stm32wb0x_it.c Drivers/MCU_Support/Src/stm32wb0x_hal_msp.c
        Drivers/MCU_Support/Src/sysmem.c Drivers/MCU_Support/Src/syscalls.c
        Drivers/MCU_Support/Src/system_stm32wb0x.c Drivers/CMSIS_Device/Source/Templates/gcc/startup_stm32wb09.s
)
set(COMMS_HAL_SOURCES
        stm32wb0x_hal_rcc.c stm32wb0x_hal_rcc_ex.c stm32wb0x_hal_flash.c stm32wb0x_hal_flash_ex.c
        stm32wb0x_hal_gpio.c stm32wb0x_hal_dma.c stm32wb0x_hal_pwr.c stm32wb0x_hal_pwr_ex.c
        stm32wb0x_hal_cortex.c stm32wb0x_hal.c
        stm32wb0x_hal_uart.c stm32wb0x_hal_uart_ex.c stm32wb0x_hal_pka.c stm32wb0x_hal_radio.c
        stm32wb0x_hal_radio_timer.c stm32wb0x_hal_rng.c stm32wb0x_hal_rtc.c stm32wb0x_hal_rtc_ex.c
        stm32wb0x_hal_tim.c stm32wb0x_hal_tim_ex.c
)
list(TRANSFORM COMMS_HAL_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/HAL/Src/")
set(COMMS_BLE_SOURCES
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/evt_handler/src/ble_evt.c
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/queued_writes/src/eatt_pwrq.c
        Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/stack/config/ble_stack_user_cfg.c
)
set(COMMS_UTILITIES_SOURCES
        Drivers/BLE_Stack/Utilities/trace/adv_trace/stm32_adv_trace.c
        Drivers/BLE_Stack/Utilities/misc/stm32_mem.c Drivers/BLE_Stack/Utilities/sequencer/stm32_seq.c
        Drivers/BLE_Stack/Utilities/lpm/tiny_lpm/stm32_lpm.c
)
set(COMMS_BLE_COMMON_SOURCES
        Drivers/BLE_Stack/Projects/Common/BLE/Startup/cpu_context_switch.s
        Drivers/BLE_Stack/Projects/Common/BLE/Startup/device_context_switch.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Flash/flash_manager.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Flash/flash_driver.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/NVMDB/Src/nvm_db_conf.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/NVMDB/Src/nvm_db.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/PKAMGR/Src/pka_manager.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Profiles/Src/gatt_profile.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/Profiles/Src/gap_profile.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/RADIO_utils/Src/RADIO_utils.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/RTDebug/RTDebug.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/osal_memcpy.s
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/blue_unit_conversion.s
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/stm_list.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/osal.c
        Drivers/BLE_Stack/Projects/Common/BLE/Modules/miscutil.c
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces/stm32_lpm_if.c
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces/hw_rng.c
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces/hw_pka.c
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces/hw_aes.c
        Drivers/BLE_Stack/Projects/Common/BLE/Interfaces/cryptolib_hw_aes.c
)
foreach(COMMS_SOURCE_LIST IN ITEMS COMMS_APP_SOURCES COMMS_MCU_SUPPORT_SOURCES COMMS_BLE_SOURCES COMMS_UTILITIES_SOURCES COMMS_BLE_COMMON_SOURCES)
        list(TRANSFORM ${COMMS_SOURCE_LIST} PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
endforeach()

target_sources(${COMMS_TARGET} PRIVATE ${COMMS_APP_SOURCES} ${COMMS_MCU_SUPPORT_SOURCES}
        ${COMMS_HAL_SOURCES} ${COMMS_BLE_SOURCES} ${COMMS_UTILITIES_SOURCES} ${COMMS_BLE_COMMON_SOURCES})
target_include_directories(${COMMS_TARGET} PRIVATE ${COMMS_INCLUDE_DIRS})
target_compile_definitions(${COMMS_TARGET} PRIVATE USE_FULL_LL_DRIVER USE_HAL_DRIVER STM32WB09 $<$<CONFIG:Debug>:DEBUG>)
target_compile_options(${COMMS_TARGET} PRIVATE -mcpu=cortex-m0plus -mthumb
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-fdata-sections;-ffunction-sections;-fstack-usage>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions;-fno-threadsafe-statics>
        $<$<COMPILE_LANGUAGE:ASM>:-x;assembler-with-cpp;-MMD;-MP>)
target_link_options(${COMMS_TARGET} PRIVATE -mcpu=cortex-m0plus -mthumb
        -T${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU_Support/stm32wb09_flash.ld --specs=nano.specs
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${COMMS_TARGET}.map -Wl,--gc-sections -Wl,--print-memory-usage)
target_link_directories(${COMMS_TARGET} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/cryptolib
        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BLE_Stack/Middlewares/ST/STM32_BLE/stack/lib)
target_link_libraries(${COMMS_TARGET} PRIVATE :libcrypto.a :stm32wb0x_ble_stack.a m)
set_target_properties(${COMMS_TARGET} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${COMMS_TARGET}.map)
