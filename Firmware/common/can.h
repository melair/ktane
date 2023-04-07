#ifndef CAN_H
#define	CAN_H

#include <stdbool.h>

/* Statistics structure for CAN bus. */
typedef struct {
    uint16_t tx_packets;
    uint16_t rx_packets;

    uint16_t tx_errors;
    uint16_t rx_errors;

    uint16_t tx_overflow;
    uint16_t rx_overflow;

    uint16_t tx_not_ready;

    uint16_t can_error;
    uint16_t id_cycles;
} can_statistics_t;

void can_initialise(void);
void can_send(uint8_t prefix, uint8_t size, uint8_t *data);
void can_service(void);
uint8_t can_get_id(void);
bool can_ready(void);
uint8_t can_get_domain(void);
void can_domain_update(uint8_t domain);
can_statistics_t *can_get_statistics(void);
bool can_dirty(void);

#endif	/* CAN_H */

