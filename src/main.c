#include "main.h"

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

void LedButton(void)
{
	// State check port 1 pin 15
	if (GPIO_0->STATE & (1 << PIN_BUTTON)) 
	{
		GPIO_1->OUTPUT &= ~(1 << PIN_LED2); // Reset port 0 pin 10
	}
	else
	{
		GPIO_1->OUTPUT |= (1 << PIN_LED2); // Set port 0 pin 10
	}
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
		LedBlink();
		// Timer32 Overflow Interrupt Flag Clear
		TIMER32_0->INT_CLEAR = TIMER32_INT_OVERFLOW_M;
		// Epic Timer32 Flag Clear
		EPIC->CLEAR = 1 << EPIC_TIMER32_0_INDEX;
	}
}

uint16_t crc16_modbus(uint8_t *buffer, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
	{
        crc ^= (uint16_t)buffer[i];
        for (int j = 0; j < 8; j++)
		{
            if (crc & 0x0001)
			{
                crc >>= 1;
                crc ^= 0xA001;
            }
			else
			{
                crc >>= 1;
            }
        }
    }
    return crc;
}


int main(void)
{
	ClockInit();
	// Timer32 Init
	TIMER32_0->TOP = 16000000u;
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

	while (1)
	{
		if ((*(&ub)).flag_command_ready)
		{
			uint8_t command_input[8];
			memset(command_input, 0, 8);
			memcpy(command_input, (*(&ub)).pointer_finished_string + 1, 8);
			(*(&ub)).flag_command_ready = 0;
			// TODO: Parse Command Here

			// (!!!) Debug Echo
			for (int i = 0; i < 6; i++)
			{
				UART_0_SendByte(command_input[i]);
			}
			// (!!!) Debug Cald and Send CRC
			uint16_t crc = crc16_modbus(command_input, 6);
			UART_0_SendByte(crc & 0xFF);
			UART_0_SendByte((crc >> 8) & 0xFF);

		}
	
		// for (volatile int i = 0; i < 100000; i++);
	}
}