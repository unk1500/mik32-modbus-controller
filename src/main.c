#include "main.h"

char* const version = FW_VERSION;

volatile uint8_t device_address = 0x01;

volatile uint32_t analog_temperature[2] = 
	{0xFFFF, 0xFFFF};
volatile uint32_t analog_humidity[2] = 
	{0xFFFF, 0xFFFF};

volatile uint32_t flag_measurement = 0;

extern volatile struct usart_modbus_buffer ub;

void ClockInit(void)
{
	// GPIO Clocks Init
	PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_GPIO_0_M | PM_CLOCK_APB_P_GPIO_1_M;

	// MCU Clocks Init
	PM->CLK_APB_M_SET |= 
		// Output Controller clocks enable
		PM_CLOCK_APB_M_PAD_CONFIG_M | 
		// Wake Up Block clocks enable
		PM_CLOCK_APB_M_WU_M | 
		// Power Manager Block clocks enable
		PM_CLOCK_APB_M_PM_M |
		// Timer32 clock enable
		PM_CLOCK_APB_M_TIMER32_0_M |
		// EPIC clocks enable
		PM_CLOCK_APB_M_EPIC_M;
}

void EnableInterrupts(void)
{
	set_csr(mstatus, MSTATUS_MIE);
	set_csr(mie, MIE_MEIE);
}

void LedBlink(void)
{
	GPIO_0->OUTPUT ^= 1 << PIN_LED1; // Toggle port 0 pin 3
}

void trap_handler()
{	
	// xprintf("EPIC->STATUS = %x\r\n", EPIC->STATUS);
	// xprintf("EPIC->RAW_STATUS = %x\r\n", EPIC->RAW_STATUS);
	if (EPIC->RAW_STATUS & (1 << EPIC_UART_0_INDEX))
	{
		UART_0_IRQHandler();
		EPIC->CLEAR = 1 << EPIC_UART_0_INDEX;
	}
	else if (EPIC->RAW_STATUS & (1 << EPIC_TIMER32_0_INDEX))
	{
		flag_measurement = 1;
		LedBlink();
		// Timer32 Overflow Interrupt Flag Clear
		TIMER32_0->INT_CLEAR = TIMER32_INT_OVERFLOW_M;
		// Epic Timer32 Flag Clear
		EPIC->CLEAR = 1 << EPIC_TIMER32_0_INDEX;
	}
}

int main(void)
{
	ClockInit();
	// Timer32 Init
	TIMER32_0->TOP = 32000000;
	TIMER32_0->INT_MASK = TIMER32_INT_OVERFLOW_M;
	// Epic Init
	EPIC->MASK_EDGE_CLEAR = 0xFFFF;
	EPIC->MASK_LEVEL_CLEAR = 0xFFFF;
	EPIC->CLEAR = 0xFFFF;
	// Set Timre32 Mask Interrupt
	EPIC->MASK_LEVEL_SET = 1 << EPIC_TIMER32_0_INDEX;
	// Set UART0 Mask Interrupt
    EPIC->MASK_LEVEL_SET = (1 << EPIC_UART_0_INDEX);

	// UART_0 (Modbus RTU) Init
	UART_0_Init();

	// Interrupts Enable 
	EnableInterrupts();

	// Timer32 Enable
	TIMER32_0->ENABLE = TIMER32_ENABLE_TIM_EN_M;

	// Relay GPIO Pins Init
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_RELAY1));
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_RELAY2));
	GPIO_0->DIRECTION_OUT = (1 << PIN_RELAY1);
	GPIO_0->DIRECTION_OUT = (1 << PIN_RELAY2);

	// RS-485 REDE Pin Init
    GPIO_1->OUTPUT &= ~(1 << PIN_REDE);
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_REDE));
	GPIO_1->DIRECTION_OUT = (1 << PIN_REDE);

	// GPIO port 0 pin 3 (LED1) Init as Output
	PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * PIN_LED1)); // Clear port 0 pin 3 settings
	GPIO_0->DIRECTION_OUT = (1 << PIN_LED1); // Set port 0 pin 3 direction as output

	// GPIO port 1 pin 3 (LED2) Init as Output
	PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * PIN_LED2)); // Clear port 1 pin 3 settings
	GPIO_1->DIRECTION_OUT = (1 << PIN_LED2); // Set port 1 pin 3 direction as output

	while (1)
	{
		if (ub.flag_command_ready)
		{
			uint8_t command_input[8];
			memset(command_input, 0, 8);
			memcpy(command_input, ub.pointer_finished_string + 1, 8);
			ub.flag_command_ready = 0;
			ParseAndAnswer(command_input);
		}

		if (flag_measurement == 1)
		{
			// Outdoor Sensor
			DHT22_Read(PIN_DHT1, &analog_temperature[0], &analog_humidity[0]);
			// Indoor Sensor
			DHT22_Read(PIN_DHT2, &analog_temperature[1], &analog_humidity[1]);

			flag_measurement = 0;
		}
	}
}