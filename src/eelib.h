#ifndef PERIPHERALS_EEPROM_H
#define PERIPHERALS_EEPROM_H

uint32_t swap_word(uint32_t);

void EEPROM_Init(void);
uint8_t EEPROM_Read_DevAddress(void);
uint32_t EEPROM_Write_DevAddress(uint8_t);

void EEPROM_Read(uint16_t, uint32_t *, uint8_t);
int32_t EEPROM_Erase(uint16_t, uint8_t);
int32_t EEPROM_Write(uint16_t, uint32_t *, uint8_t);

#endif