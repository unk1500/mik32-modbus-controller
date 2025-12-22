#include "main.h"

struct usart_modbus_buffer ub;

void SwitchUBString(struct usart_modbus_buffer *ub)
{
	if ((*ub).pointer_receiving_string == &(*ub).buffer0.size)
	{
		(*ub).buffer1.size = 0;
		(*ub).pointer_receiving_string = &(*ub).buffer1.size;
		(*ub).pointer_finished_string = &(*ub).buffer0.size;
	}
	else
	{
		(*ub).buffer0.size = 0;
		(*ub).pointer_receiving_string = &(*ub).buffer0.size;
		(*ub).pointer_finished_string = &(*ub).buffer1.size;
	}
	(*ub).flag_command_ready = 1;
}

void UART_0_Init()
{
    // Init UART Modbus Buffer
    ub.pointer_receiving_string = &ub.buffer0.size;
    ub.buffer0.size = 0;
    ub.buffer1.size = 0;
    memset((void *)ub.buffer0.buffer, 0, 128);
	memset((void *)ub.buffer1.buffer, 0, 128);

    // Set UART0 GPIO registers
    // (!) PO Clock enable in main.c
    // UART0 RX: P0.5
    PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * 5));
	PAD_CONFIG->PORT_0_CFG |= 0b01 << (2 * 5);
    // UART0 TX: P0.6
    PAD_CONFIG->PORT_0_CFG &= ~(0b11 << (2 * 6));
	PAD_CONFIG->PORT_0_CFG |= 0b01 << (2 * 6);

    // Set UART0 control registers
    PM->CLK_APB_P_SET |= PM_CLOCK_APB_P_UART_0_M;
    UART_0->CONTROL2 = 0;
    UART_0->CONTROL3 = 0;
    UART_0->FLAGS = 0xFFFFFFFF;
    UART_0->DIVIDER = OSC_SYSTEM_VALUE / 115200;
    UART_0->CONTROL1 = UART_CONTROL1_RXNEIE_M | UART_CONTROL1_TE_M | UART_CONTROL1_RE_M | UART_CONTROL1_UE_M;

    while ((UART_0->FLAGS & UART_FLAGS_REACK_M) == 0);
    while ((UART_0->FLAGS & UART_FLAGS_TEACK_M) == 0);
}

void UART_0_IRQHandler()
{
    if (UART_0->FLAGS & UART_FLAGS_RXNE_M) // (!!!) Maybe add !ORE Flag here?
    {
        // Read Data and Clear RXNE Flag
        uint8_t data = UART_0->RXDATA;
   		// UART_0_SendByte(data);
        
        // Data Buffer Address + Data Count + Size of Data Count = New Byte
        *(uint8_t *)(
            (uint8_t *)ub.pointer_receiving_string + 
            *(uint8_t *)ub.pointer_receiving_string + 
            4
        ) = data;
        // Increment Data Count
		++*ub.pointer_receiving_string;

        if ((*ub.pointer_receiving_string == 8) & (*ub.pointer_receiving_string != 0))
		    SwitchUBString(&ub);
    }
}

void UART_0_SendByte(uint8_t data)
{
    while ((UART_0->FLAGS & UART_FLAGS_TXE_M) == 0);
    UART_0->TXDATA = data;
}