#ifndef WIRES_H
#define	WIRES_H

void wires_initialise(void);

#define WIRES_COUNT 6

typedef struct {
    bool complex;

    struct {
        unsigned adc_check  :3;
        unsigned init_stage :3;
        unsigned animation_tick :4;
    } process;

    struct {
        unsigned required       :1;
        unsigned updated        :1;

        unsigned cutneeded      :1;
        unsigned cut            :1;

        unsigned light_left     :1;
        unsigned light_right    :1;

        uint8_t wiretype;
        uint16_t adc_value;
    } wires[WIRES_COUNT];
} mode_wires_t;

#endif	/* WIRES_H */
