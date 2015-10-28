/* UniDOS emulator */
/* By Nguyen Anh Quynh, 2015 */

#include "int20.h"

void int20(uc_engine *uc) //terminate process
{
    uc_emu_stop(uc);
    //printf(">>> 0x%x: interrupt: %x, AH = %02x\n", r_ip, 0x20, r_ah);
}
