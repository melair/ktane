#ifndef POLYFILL_PIC_H
#define POLYFILL_PIC_H

#include <xc.h>

#if defined(_PIC18F15Q40_H_)
#include "pic18f15q40.h"
#elif defined(_PIC18F25Q43_H_)
#include "pic18f25q43.h"
#elif defined(_PIC18F57Q84_H_)
#include "pic18f57q84.h"
#else
#error Unknown PIC for polyfill.
#endif

#endif