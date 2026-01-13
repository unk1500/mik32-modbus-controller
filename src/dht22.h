#ifndef PERIPHERALS_DHT22_H
#define PERIPHERALS_DHT22_H

__attribute__ ((section(".ram_text"))) void DHT22_Read(uint16_t, volatile uint32_t *, volatile uint32_t *);

#endif