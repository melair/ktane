#ifndef NODE_LINK_H
#define NODE_LINK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool NodeLink_Init(void);

void NodeLink_Service(void);

void NodeLink_Front_IRQHandler(void);

void NodeLink_Rear_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_LINK_H
