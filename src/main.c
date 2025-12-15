#include <mik32_memory_map.h>
#include <pad_config.h>
#include <gpio.h>
#include <power_manager.h>
#include <wakeup.h>

#include "uart_lib.h"
#include "xprintf.h"
#include "scr1_timer.h"

#define	MIK32V2

#define PIN_LED1 3	 // LED1 is PORT_0_3
#define PIN_LED2 3	 // LED2 is PORT_1_3
#define PIN_BUTTON 8	 // BUTTON is PORT_0_8

void InitClock(void)
{
	// GPIO Clocks Init
	PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M | PM_CLOCK_APB_P_GPIO_0_M | PM_CLOCK_APB_P_GPIO_1_M;

	// MCU Clocks Init
	PM->CLK_APB_M_SET |= PM_CLOCK_APB_M_PAD_CONFIG_M | PM_CLOCK_APB_M_WU_M | PM_CLOCK_APB_M_PM_M;
}

void ledBlink(void)
{
	GPIO_0->OUTPUT ^= 1 << PIN_LED1; // Toggle port 0 pin 3
}

void ledButton(void)
{
	// State check port 1 pin 15
	if (GPIO_0->STATE & (1 << PIN_BUTTON)) {
		GPIO_1->OUTPUT |= (1 << PIN_LED2); // Set port 0 pin 10
	} else {
		GPIO_1->OUTPUT &= ~(1 << PIN_LED2); // Reset port 0 pin 10
	}
}

int main(void)
{
	InitClock();

	// DEBUG UART (UART0) Init
	UART_Init(UART_0, OSC_SYSTEM_VALUE/115200, UART_CONTROL1_TE_M | UART_CONTROL1_RE_M |
						   UART_CONTROL1_M_8BIT_M, 0, 0);

	// GPIO port 0 pin 3 (LED1) Init as Output
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_LED1)); // Clear port 0 pin 3 settings
	GPIO_0->DIRECTION_OUT = (1 << PIN_LED1); // Set port 0 pin 3 direction as output

	// GPIO port 1 pin 3 (LED2) Init as Output
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_LED2)); // Clear port 1 pin 3 settings
	GPIO_1->DIRECTION_OUT = (1 << PIN_LED2); // Set port 1 pin 3 direction as output

	// GPIO port 0 pin 8 (BUTTON) Init as Input
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_BUTTON)); // Clear port 0 pin 8 settings
	GPIO_0->DIRECTION_IN = (1 << PIN_BUTTON); // Set port 0 pin 8 direction as input

	int counter = 0;

	while (1) {
		ledBlink(); /* Toggle LED1 */

		ledButton(); /* Set LED2 if BUTTON is pressed */

		xprintf("Hello, world! Counter = %d\r\n", counter++);
	
		for (volatile int i = 0; i < 100000; i++);
	}
}

