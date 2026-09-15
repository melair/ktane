#ifndef EDGEWORK_DATA_H
#define EDGEWORK_DATA_H

#include <stdint.h>

/* Must be 8 bit aligned for protocol transmission. */
#pragma pack(push, 1)

typedef struct {
    union {
        struct {
            uint8_t value[6];
        } serial;

        struct {
            uint8_t indicator;

            struct {
                unsigned lit :1;
            } flags;
        } indicator;

        struct {
            uint8_t count;
        } batteries;

        struct {
            unsigned parallel :1;
            unsigned serial :1;
            unsigned dvi :1;
            unsigned ps2 :1;
            unsigned rj45 :1;
            unsigned stereo_rca :1;
        } ports;

        struct {
            uint8_t value[6];

            struct {
                unsigned t1 :1;
                unsigned t2 :1;
                unsigned t3 :1;
                unsigned t4 :1;

                unsigned col1 :1;
                unsigned dot :1;
            } icons;

            struct {
                unsigned active_display :1;
                unsigned flashing :1;
            } flags;
        } twofa;

        struct {
            struct {
                unsigned powered :1;
            } flags;
        } controller;
    };

    unsigned identify :1;
} edgework_state_t;

#pragma pack(pop)

#endif // EDGEWORK_DATA_H
