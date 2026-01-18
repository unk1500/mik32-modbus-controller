#ifndef MAIN_H
#define MAIN_H

#include <string.h>

#include <mik32_memory_map.h>
#include <scr1_csr_encoding.h>
#include <scr1_timer.h>
#include <csr.h>
#include <epic.h>
#include <pad_config.h>
#include <gpio.h>
#include <uart.h>
#include <eeprom.h>
#include <power_manager.h>
#include <wakeup.h>
#include <timer32.h>

#include "usart.h"
#include "dht22.h"
#include "eelib.h"
#include "xprintf.h"

#define	MIK32V2

#define OSC_SYSTEM_VALUE ((uint32_t)32000000U)

// Modbus speed is 115200, timeout criteria is 3,5 words
// Timeout_Value = (1 / 115200) * (8 data bytes + 1 start byte + 1 stop byte) * 3.5 = 0.000303819 s
// Timer_Top_Values = 0.000303819 / (1 / 32M) = 0d9722
#define MB_TIMEOUT_VALUE ((uint32_t)9722)

// Registers Parameters
#define DO_COUNT 2	 	            // Coils Count
#define DI_COUNT 1	 	            // Discrete Inputs Count
#define AI_COUNT 2	 	            // Analog Inputs Count
#define AI_HUMIDITY_OFFSET 0x190    // Humidity Registers Offset

// DHT22 Sensors Pins (PORT_1)
#define PIN_DHT1 11	 	            // DHT22 at PORT_1_11
#define PIN_DHT2 7	 	            // DHT22 at PORT_1_7

// Relay Pins (PORT_0)
#define PIN_RELAY1 0
#define PIN_RELAY2 1

// Digital Input Pins (PORT_2)
#define PIN_REED 7

// RS-485 Pins (PORT_1)
#define PIN_REDE 0

// Debug Pins
#define PIN_LED1 3	 	            // LED1 is PORT_0_3
#define PIN_LED2 3	 	            // LED2 is PORT_1_3
#define PIN_BUTTON 8	            // BUTTON is PORT_0_8

// Debug Pins
#define EE_ADDRESS 0x1F80
#define EE_DEVADDR_OFFSET 0
#define EE_DEVADDR_DEFAULT 0x01

#endif