#ifndef BLE_HPP
#define BLE_HPP

#include <BLEAdvertising.h>

extern BLEAdvertising *pAdvertising;  // Declare as extern
extern uint32_t delayMilliseconds;    // Declare as extern

void bleSetup();
void bleLoop();

#endif
