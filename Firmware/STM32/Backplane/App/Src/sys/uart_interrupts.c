#include "chassis_bus.h"
#include "node_link.h"

void USART1_IRQHandler(void) {
    NodeLink_Front_IRQHandler();
}

void USART3_4_IRQHandler(void) {
    ChassisBus_IRQHandler();
    NodeLink_Rear_IRQHandler();
}
