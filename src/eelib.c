#include "main.h"

uint32_t swap_word(uint32_t word) {
    return ((word & 0xFF000000) >> 24) |
           ((word & 0x00FF0000) >> 8)  |
           ((word & 0x0000FF00) << 8)  |
           ((word & 0x000000FF) << 24);
}

void EEPROM_Init()
{
    EEPROM_REGS->EECON = 0;

    // Calculate Timings based on HAL lib
    int32_t frequency = OSC_SYSTEM_VALUE;

    // Set N_LD, N_R_2 default values
    uint32_t ncycrl = EEPROM_REGS->NCYCRL;
    ncycrl &= ~EEPROM_NCYCRL_N_LD_M;
    ncycrl |= (1 << EEPROM_NCYCRL_N_LD_S);
    ncycrl &= ~EEPROM_NCYCRL_N_R_2_M;
    ncycrl |= (1 << EEPROM_NCYCRL_N_R_2_S);

    // Calc N_R_1, N_EP_1, N_EP_2 values
    const int32_t N_EP_1_denominator = 1000 * 1000 * 1000 / (2 * 1000 * 1000);
    const int32_t N_EP_2_reduce = 5 * 1000;
    const int32_t N_EP_2_numerator = 15 * 1000 / N_EP_2_reduce;
    const int32_t N_EP_2_denominator = 1000 * 1000 * 1000 / N_EP_2_reduce;

    uint32_t ncycep1 = EEPROM_REGS->NCYCEP1;
    ncycep1 &= ~EEPROM_NCYCEP1_N_EP_1_M;
    ncycep1 = frequency / N_EP_1_denominator;
    EEPROM_REGS->NCYCEP1 = ncycep1;

    uint32_t ncycep2 = EEPROM_REGS->NCYCEP2;
    ncycep2 &= ~EEPROM_NCYCEP2_N_EP_2_M;
    ncycep2 = (frequency * N_EP_2_numerator) / N_EP_2_denominator;
    EEPROM_REGS->NCYCEP2 = ncycep2;

    int32_t n_r_1_step_1 = (frequency / 1000) * 51;
    int32_t n_r_1_step_2 = n_r_1_step_1 / (1000 * 1000);

    if ((n_r_1_step_1 % (1000 * 1000)) != 0)
        n_r_1_step_2 += 1;
    
    ncycrl &= ~EEPROM_NCYCRL_N_R_1_M;
    ncycrl |= (n_r_1_step_2 << EEPROM_NCYCRL_N_R_1_S);
    EEPROM_REGS->NCYCRL = ncycrl;

    return;
}

uint8_t EEPROM_Read_DevAddress()
{
    uint8_t eeprom_devaddr;
    uint32_t data[4];

    EEPROM_Read(EE_ADDRESS, data, 4);
    eeprom_devaddr = swap_word(data[0]);
    // (!!!) DEBUG
    xprintf("First Read DevAddr: %02X\r\n", eeprom_devaddr);

    if ((eeprom_devaddr == 0x00) || (eeprom_devaddr > 247))
    {
        if (!EEPROM_Write_DevAddress(EE_DEVADDR_DEFAULT))
        {
            EEPROM_Read(EE_ADDRESS, data, 4);
            eeprom_devaddr = swap_word(data[0]);
            // (!!!) DEBUG
            xprintf("Second Read DevAddr: %02X\r\n", eeprom_devaddr);

            return eeprom_devaddr;
        }
        else
            return EE_DEVADDR_DEFAULT;
    }
    else
        return eeprom_devaddr;
}

uint32_t EEPROM_Write_DevAddress(uint8_t new_address)
{
    uint32_t data[32];

    if (!EEPROM_Erase(EE_ADDRESS, 32))
    {
        for (int i = 0; i < 32; i++)
            data[i] = 0;
        data[EE_DEVADDR_OFFSET >> 2] = swap_word(new_address);
        
        EEPROM_Write(EE_ADDRESS, data, 32);
    }
    else
        return -1;

    return 0;
}

// Read EEPROM
void EEPROM_Read(uint16_t address, uint32_t *data, uint8_t length)
{
  
    EEPROM_REGS->EEA = address;

    for (uint32_t i = 0; i < length; i++)
    {
        data[i] = EEPROM_REGS->EEDAT;
        // (!!!) Debug Output
        // xprintf("%02d: %08X\r\n", i, EEPROM_REGS->EEDAT);
    }

    return;
}

// Erase EEPROM
int32_t EEPROM_Erase(uint16_t address, uint8_t length)
{
    EEPROM_REGS->EECON |= (1 << EEPROM_EECON_BWE_S);

    EEPROM_REGS->EEA = address;
    for (int i = 0; i < length; i++)
        EEPROM_REGS->EEDAT = 0;
    
    EEPROM_REGS->EECON |= EEPROM_EECON_OP(EEPROM_EECON_OP_ER) | EEPROM_EECON_EX_M;

    int32_t timeout = 100000;
    while (timeout)
    {
        timeout--;
        if (!(EEPROM_REGS->EESTA & EEPROM_EESTA_BSY_M))
            return 0;
    }

    return -1;
}

// Write EEPROM
int32_t EEPROM_Write(uint16_t address, uint32_t *data, uint8_t length)
{
    EEPROM_REGS->EECON |= EEPROM_EECON_BWE_M;

    EEPROM_REGS->EEA = address;
    for (int i = 0; i < length; i++)
        EEPROM_REGS->EEDAT = data[i];
    
    EEPROM_REGS->EECON |= EEPROM_EECON_OP(EEPROM_EECON_OP_PR) | EEPROM_EECON_EX_M;

    int32_t timeout = 100000;
    while (timeout)
    {
        timeout--;
        if (!(EEPROM_REGS->EESTA & EEPROM_EESTA_BSY_M))
            return 0;
    }

    return -1;
}