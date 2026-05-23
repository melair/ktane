#ifndef MODE_STORAGE_H
#define MODE_STORAGE_H

#include "blank/blank.h"
#include "chassis/chassis.h"
#include "timer/timer.h"
#include "simon/simon.h"
#include "rfid/rfid.h"
#include "whosonfirst/whosonfirst.h"

typedef struct {
  union {
    blank_t blank;
    chassis_t chassis;
    timer_t timer;
    simon_t simon;
    rfid_t rfid;
    whosonfirst_t whosonfirst;
  };
} mode_storage_t;

extern mode_storage_t mode_storage;

#endif