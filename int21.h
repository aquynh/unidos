#ifndef DOSCORN_INT21_H
#define DOSCORN_INT21_H

#include <unicorn/unicorn.h>

// real-mode far pointer created from segment & offset
#define MK_FP(SEG, OFF) (((SEG) << 4) + (OFF))

#define MIN(a, b) ((a) < (b)? (a) : (b))

void int21_init(void);
void int21(uc_engine *uc);

#endif
