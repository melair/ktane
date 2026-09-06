#include "chassis_bus.h"
#include "module_link.h"

void USART1_IRQHandler(void) {
    ModuleLink_Front_IRQHandler();
}

void USART3_4_IRQHandler(void) {
    ChassisBus_IRQHandler();
    ModuleLink_Rear_IRQHandler();
}
