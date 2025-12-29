#include "main.h"

extern const char* const version;
extern volatile int32_t analog_temperature[2];
extern volatile int32_t analog_humidity[2];

struct usart_modbus_buffer ub;

uint16_t CrcModbus(uint8_t *buffer, uint16_t length)
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

void ParseAndAnswer(uint8_t *command_input)
{
    uint8_t answer_output[10];
    uint8_t answer_size = 0;
    uint16_t reg_address;

    // (!) Replace with real address
    answer_output[0] = 0x01;
    // (!) Replace with real address
    if (command_input[0] != 1)
        return;
    else
    {
        switch (command_input[1])
        {
            case 0x01:
                // Read Coil Status
                if ((command_input[2] != 0) || (command_input[3] > DO_COUNT - 1))
                    // Error code 0x02: Illegal Data Address
                    return;
                else
                {
                    
                }
                break;
            case 0x02:
                // Read Input Status
                break;
            case 0x03:
                // Read Holding Registers
                reg_address = (command_input[2] << 8) | command_input[3];

                // Check version register address
                if (reg_address == 0x7D6)
                {
                    answer_output[2] = 0x04;
                    // 'v' char code
                    answer_output[3] = 0x76;
                    // version string chars
                    answer_output[4] = version[0];
                    answer_output[5] = version[1];
                    answer_output[6] = version[2];
                    answer_size = 7;
                }
                break;
            case 0x04:
                // Read Input Registers
                reg_address = (command_input[2] << 8) | command_input[3];

                if (reg_address < AI_COUNT)
                {
                    // Temperature addresses
                    answer_output[2] = 0x02;
                    answer_output[3] = (analog_temperature[reg_address] >> 8) & 0xFF;
                    answer_output[4] = analog_temperature[reg_address] & 0xFF;
                    answer_size = 5;
                    
                }
                else if (reg_address - AI_HUMIDITY_OFFSET < AI_COUNT)
                {
                    // Humidity addresses
                    reg_address -= AI_HUMIDITY_OFFSET;
                    answer_output[2] = 0x02;
                    answer_output[3] = (analog_humidity[reg_address] >> 8) & 0xFF;
                    answer_output[4] = analog_humidity[reg_address] & 0xFF;
                    answer_size = 5;
                }
                else
                {
                    // Error code 0x02: Illegal Data Address
                    return;
                }
                break;
            case 0x05:
                // Force Single Coil
                break;
            case 0x06:
                // Preset Single Register
            defaulf:
                // Error code 0x01: Illegal Function
                return;
        }
    }
    answer_output[1] = command_input[1];

    // (!!!) Debug Echo
    // for (int i = 0; i < 6; i++)
    // {
    //     UART_0_SendByte(command_input[i]);
    // }
    // // (!!!) Debug Calc and Send CRC
    // uint16_t crc = CrcModbus(command_input, 6);
    // UART_0_SendByte(crc & 0xFF);
    // UART_0_SendByte((crc >> 8) & 0xFF);

    // for (int i = 0; i < 6; i++)
    // {
    //     UART_0_SendByte(command_input[i]);
    // }
    // (!!!) Debug Calc and Send CRC
    uint16_t crc = CrcModbus(answer_output, answer_size);
    answer_output[answer_size] = crc & 0xFF;
    answer_output[answer_size + 1] = (crc >> 8) & 0xFF;

    for (int i = 0; i < answer_size + 2; i++)
    {
        UART_0_SendByte(answer_output[i]);
    }

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