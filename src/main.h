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
#include <power_manager.h>
#include <wakeup.h>
#include <timer32.h>

#include "usart.h"
#include "xprintf.h"

#define	MIK32V2

#define OSC_SYSTEM_VALUE ((uint32_t)32000000U)

#define DO_COUNT 2	 	            // Coils Count
#define DI_COUNT 1	 	            // Discrete Inputs Count
#define AI_COUNT 2	 	            // Analog Inputs Count
#define AI_HUMIDITY_OFFSET 0x190    // Humidity Registers Offset

#define PIN_LED1 3	 	            // LED1 is PORT_0_3
#define PIN_LED2 3	 	            // LED2 is PORT_1_3
#define PIN_BUTTON 8	            // BUTTON is PORT_0_8

#endif