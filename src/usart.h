#ifndef PERIPHERALS_USART_H
#define PERIPHERALS_USART_H

// Support only 8 bytes commands:
// 0x01: Digital Output (DO) Read           (Read Coil Status)
// 0x02: Digital Input (DI) Read            (Read Input Status)
// 0x03: Analog Output (AO) Read            (Read Holding Registers)
// 0x04: Analog Input (AI) Read             (Read Input Registers)
// 0x05: Single Digital Output (DO) Write   (Force Single Coil)
// 0x06: Single Analog Output (AO) Write    (Preset Single Register)
//
// Input (from master) packet format:
// 00: Slave Address (0x01 default)
// 01: Command Code (0x01 default)
// 02: High Register/Coil Address
// 03:  Low Register/Coil Address
// 04: High Data/Count Value
// 05:  Low Data/Count Value
// 06: High CRC
// 07:  Low CRC

struct usart_modbus_command
{
    uint32_t size;
    uint8_t buffer[128];
};

struct usart_modbus_buffer
{
    uint32_t flag_command_ready;
    uint32_t *pointer_receiving_string;
    uint32_t *pointer_finished_string;
    struct usart_modbus_command buffer0;
    struct usart_modbus_command buffer1;
};

void UART_0_Init(void);
void UART_0_IRQHandler(void);
void UART_0_SendByte(uint8_t data);
void UART_0_SendAnswer(volatile uint8_t *, uint32_t);
uint16_t CrcModbus(volatile uint8_t *, uint16_t);
void ParseAndAnswer(uint8_t *);

#endif