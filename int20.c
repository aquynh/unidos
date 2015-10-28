/* UniDOS emulator */
/* By Nguyen Anh Quynh, 2015 */

#include "int20.h"

void int20(uc_engine *uc)
{
    uint16_t r_ip;
    uint8_t r_ah;

    uc_reg_read(uc, UC_X86_REG_AH, &r_ah);
    uc_reg_read(uc, UC_X86_REG_IP, &r_ip);

    printf(">>> 0x%x: interrupt: %x, AH = %02x\n", r_ip, 0x20, r_ah);
}
