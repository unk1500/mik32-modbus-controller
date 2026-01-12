#ifndef PERIPHERALS_EEPROM_H
#define PERIPHERALS_EEPROM_H

void EEPROM_Init(void);
uint8_t EEPROM_Read_DevAddress(void);
void EEPROM_Write_DevAddress(uint8_t);

#endif