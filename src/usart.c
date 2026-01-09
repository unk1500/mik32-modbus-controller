#include "main.h"

extern const char* const version;
extern volatile int8_t device_address;
extern volatile int32_t analog_temperature[2];
extern volatile int32_t analog_humidity[2];

struct usart_modbus_buffer ub;

uint16_t CrcModbus(volatile uint8_t *buffer, uint16_t length)
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
    uint16_t data_count;
    uint32_t pin_number;

    // Device address
    answer_output[0] = device_address;
    // Device address check
    if (command_input[0] != device_address)
        return;
    else
    {
        reg_address = (command_input[2] << 8) | command_input[3];
        switch (command_input[1])
        {
            // Read Coil Status
            case 0x01:
                data_count = (command_input[4] << 8) | command_input[5];

                if (reg_address + data_count - 1 > DO_COUNT - 1)
                {
                    // Error code 0x02: Illegal Data Address
                    answer_output[2] = 0x81;
                    answer_output[3] = 0x02;
                    answer_size = 3;
                    UART_0_SendAnswer(answer_output, answer_size);
                    return;
                }
                else
                {
                    answer_output[2] = 0x01;
                    answer_output[3] = 0x00;
                    // Read both of relay
                    if (data_count == 2)
                    {
                        answer_output[3] |= (GPIO_0->STATE & (1 << PIN_RELAY1)) ? 1 : 0;
                        answer_output[3] |= ((GPIO_0->STATE & (1 << PIN_RELAY2)) ? 1 : 0) << 1;
                    }
                    // Read one of relay
                    else
                    {
                        if (reg_address == 0)
                            pin_number = PIN_RELAY1;
                        else
                            pin_number = PIN_RELAY2;

                        answer_output[3] |= ((GPIO_0->STATE & (1 << pin_number)) ? 1 : 0);
                    }
                    answer_size = 4;
                }
                break;
            // Read Input Status
            case 0x02:
                // (!!!)
                break;
            // Read Holding Registers
            case 0x03:
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
                // Check device address register address
                else if (reg_address == 0x00)
                {
                    answer_output[2] = 0x02;
                    answer_output[3] = 0x00;
                    answer_output[4] = device_address;
                    answer_size = 5;
                }
                break;
            // Read Input Registers
            case 0x04:
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
                    answer_output[2] = 0x84;
                    answer_output[3] = 0x02;
                    answer_size = 3;
                    UART_0_SendAnswer(answer_output, answer_size);
                    return;
                }
                break;
            // Force Single Coil
            case 0x05:
                if (reg_address > DO_COUNT - 1)
                {
                    // Error code 0x02: Illegal Data Address
                    answer_output[2] = 0x85;
                    answer_output[3] = 0x02;
                    answer_size = 3;
                    UART_0_SendAnswer(answer_output, answer_size);
                    return;
                }
                else
                {
                    uint16_t data_value = (command_input[4] << 8) | command_input[5];

                    if (reg_address == 0)
                        pin_number = PIN_RELAY1;
                    else
                        pin_number = PIN_RELAY2;

                    if (data_value == 0xFF00)
                    {
                        GPIO_0->OUTPUT |= (1 << pin_number);

                    }
                    else if (data_value == 0x0000)
                    {
                        GPIO_0->OUTPUT &= ~(1 << pin_number);
                    }
                    else
                    {
                        answer_output[2] = 0x85;
                        answer_output[3] = 0x03;
                        answer_size = 3;
                        UART_0_SendAnswer(answer_output, answer_size);
                        return;
                    }
                    answer_output[2] = command_input[2];
                    answer_output[3] = command_input[3];
                    answer_output[4] = command_input[4];
                    answer_output[5] = command_input[5];
                    answer_size = 6;
                }

                break;
            // Preset Single Register
            case 0x06:
                if (reg_address != 0)
                {
                    // Error code 0x02: Illegal Data Address
                    answer_output[2] = 0x86;
                    answer_output[3] = 0x02;
                    answer_size = 3;
                    UART_0_SendAnswer(answer_output, answer_size);
                    return;
                }
                else
                {
                    uint16_t data_value = (command_input[4] << 8) | command_input[5];
                    if (data_value > 247)
                    {
                        // Error code 0x03: Illegal Data Value
                        answer_output[2] = 0x86;
                        answer_output[3] = 0x03;
                        answer_size = 3;
                        UART_0_SendAnswer(answer_output, answer_size);
                        return;
                    }
                    else
                    {
                        /// (!!!) Whire address to EEPROM
                    }

                }
                break;
            defaulf:
                // Error code 0x01: Illegal Function
                answer_output[2] = 0x81;
                answer_output[3] = 0x01;
                answer_size = 3;
                UART_0_SendAnswer(answer_output, answer_size);
                return;
        }
    }
    answer_output[1] = command_input[1];

    UART_0_SendAnswer(answer_output, answer_size);
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

void UART_0_SendAnswer(volatile uint8_t *data, uint32_t size)
{
    // Calc Answer CRC
    uint16_t crc = CrcModbus(data, size);
    data[size] = crc & 0xFF;
    data[size + 1] = (crc >> 8) & 0xFF;

    // Enable REDE
    for (volatile int rede_delay = 0; rede_delay < 500; rede_delay++);
    GPIO_1->OUTPUT |= (1 << PIN_REDE);
    for (volatile int rede_delay = 0; rede_delay < 500; rede_delay++);

    // Send Answer
    for (int i = 0; i < size + 2; i++)
        UART_0_SendByte(data[i]);

    // Disable REDE
    for (volatile int rede_delay = 0; rede_delay < 500; rede_delay++);
    GPIO_1->OUTPUT &= ~(1 << PIN_REDE);
    for (volatile int rede_delay = 0; rede_delay < 500; rede_delay++);

}

void UART_0_SendByte(uint8_t data)
{
    while ((UART_0->FLAGS & UART_FLAGS_TXE_M) == 0);
    UART_0->TXDATA = data;
}