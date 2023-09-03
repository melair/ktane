# Module Port Usage
Quick reference for module and peripheral usage.

## Modules
|Module|Port A|Port B|Port C|
|--|--|--|--|
|Controller|GPIO<br />*(Code PWM)*|GPIO<br />*(Code PWM) (IO)*||
|Card Scan|||RFID<br />*(SPI)*|
|Combination|GPIO<br />*(Code PWM)*|GPIO<br />*(Code PWM) (IO)*||
|Password||Keymatrix<br />*(Keymatrix)*||
|Operator|GPIO<br />*(IO)*|||
|Maze|GPIO<br />*(IO)*|||
|Simon Says|GPIO<br />*(IO)*|GPIO<br />*(TMR1 Based PWM)*||
|Keys|GPIO<br />*(Code PWM)*|GPIO<br />*(Code PWM) (IO)*||
|Wires||GPIO<br />*(IO)*||
|Password||Keymatrix<br />*(Keymatrix)*||
|Chassis|Audio<br />*(CWG) (PWM1/PWM2)*|Power<br />*(I2C)*|SPI<br />*(SPI)*|