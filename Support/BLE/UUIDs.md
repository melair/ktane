# BLE UUIDs

## Discovery

For the purposes of BLE devices scanning and offering the bomb to the client, the we will advertise `6f5e666a-4804-4782-ab75-da0f285dd234` with no characteristics.

## Services

All services used for meaningful communication will have the follow UUID format.

`SSSSCCCC-c166-84d2-8c71-3ff05f5be867`

Where `SSSS` is a service number, and `CCCC` is the characteristic number.

### Battery Service (`SIG 0x180F`)

#### Battery Level (`SIG 0x2A19`) (Read/Notify)

uint8_t of battery level, 0 - 100.

### Information Service - `0x0000`

### Control Service - `0x0010`

#### Command Request - `0x0000` (Write)

Command requests are written to this characteristic. All commands are handled asyncronously, even if they are syncronous.

#### Command Response - `0x0001` (Indicator)

Command responses are read from this characteristic, which additionally has indications on so that clients are notified of the responses being ready.

### Game Service - `0x0020`

#### State - `0x0000` (Read/Notify)

uint8_t representing the current state of the game.

#### Timer - `0x0010` (Read/Notify)

uint32_t representing the time on the clock in ms.

#### Strikes - `0x0011` (Read/Notify)

uint8_t representing the the number of strikes occured.
uint8_t representing the number of strikes possible.