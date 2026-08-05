#ifndef NVM_H
#define NVM_H
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

#endif

#define NVM_MAGIC_ID 0x0000
#define NVM_MAGIC_LEN 6
#define NVM_MAGIC_DATA 0x14d184c6

typedef enum : uint8_t {
    MAGIC = 0,
    UINT8,
    UINT16,
} NVM_Type;

typedef struct {
    NVM_Type type;
    uint16_t id;
    void *data;
} NVM_Query;

void NVM_Init(void);

bool NVM_Read(const NVM_Query *queries, uint8_t query_count);
bool NVM_Write(const NVM_Query *query);

#ifdef __cplusplus
}
#endif

#endif //NVM_H
