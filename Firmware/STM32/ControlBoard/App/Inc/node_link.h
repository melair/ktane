#ifndef NODE_LINK_H
#define NODE_LINK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool NodeLink_Init(void);

void NodeLink_Service(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_LINK_H
