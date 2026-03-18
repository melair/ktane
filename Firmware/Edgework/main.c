#include <xc.h>
#include <hal/pin.h>
#include "gpio.h"

int main(){
    pin_config(GPIO_0, INPUT, 0);
    pin_config(GPIO_11, INPUT, 0);

    return 0;
}
