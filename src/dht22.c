#include "main.h"

/*

PORT 1_6

*/


void DHT22_Delay(int32_t value)
{
    for (volatile int delay_counter = 0; delay_counter < value; delay_counter++);
}

void DHT22_Read(uint16_t pin_number, volatile uint32_t *temperature, volatile uint32_t *humidity)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint32_t timeout;

    PAD_CONFIG->PORT_1_CFG &= ~(0b11 << (2 * pin_number));

    GPIO_1->OUTPUT &= ~(1 << pin_number);
    // Set pin as output
	GPIO_1->DIRECTION_OUT = (1 << pin_number);
    // Delay 5 ms
    DHT22_Delay(4500);

    // Set pin as input
   	GPIO_1->DIRECTION_IN = (1 << pin_number);

    // Waiting for sensor about 30 us (pin must become low)
    timeout = 0;
    while (GPIO_1->STATE & (1 << pin_number))
    {
        timeout++;
        if (timeout > 100)
            return;
    }

    // Waiting for sensor about 80 us (pin must become higt)
    timeout = 0;
    while (!(GPIO_1->STATE & (1 << pin_number)))
    {
        timeout++;
        if (timeout > 150)
            return;
    }

    // Waiting for sensor about 80 us (pin must become low)
    timeout = 0;
    while (GPIO_1->STATE & (1 << pin_number))
    {
        timeout++;
        if (timeout > 150)
            return;
    }

    // Data Receive (2 bytes temperature + 2 bytes humidity + 1 byte CRC)
    uint32_t j = 0;
    for (uint8_t bit_counter = 0; bit_counter < 40; bit_counter++)
    {
        j = bit_counter / 8;
        data[j] = data[j] << 1;

        // Waiting for sensor about 50-65 us (pin must become higt)
        timeout = 0;
        while (!(GPIO_1->STATE & (1 << pin_number)))
        {
            timeout++;
            if (timeout > 150)
                return;
        }

        // Waiting for sensor about 27-70 us (pin must become low)
        timeout = 0;
        while (GPIO_1->STATE & (1 << pin_number))
        {
            timeout++;
            if (timeout > 150)
                return;
        }

        if (timeout > 80)
            data[j] |= 0x1;
    }
    
    // Check CRC
    uint8_t crc = 0;
    for (int i = 0; i < 4; i++)
        crc += data[i];
    if (crc != data[4])
        return;

    // Return values
    *temperature = (data[2] << 8) | data[3];
    *humidity = (data[0] << 8) | data[1];
}