#ifndef __I960_H
#define __I960_H

enum {
  I960_PFP = 0,
  I960_SP  = 1,
  I960_RIP = 2,
  I960_FP  = 31,

  I960_R0 = 1,
  I960_R1 = 2,
  I960_R2 = 3,
  I960_R3 = 4,
  I960_R4 = 5,
  I960_R5 = 6,
  I960_R6 = 7,
  I960_R7 = 8,
  I960_R8 = 9,
  I960_R9 = 10,
  I960_R10 = 11,
  I960_R11 = 12,
  I960_R12 = 13,
  I960_R13 = 14,
  I960_R14 = 15,
  I960_R15 = 16,
  I960_G0 = 17,
  I960_G1 = 18,
  I960_G2 = 19,
  I960_G3 = 20,
  I960_G4 = 21,
  I960_G5 = 22,
  I960_G6 = 23,
  I960_G7 = 24,
  I960_G8 = 25,
  I960_G9 = 26,
  I960_G10 = 27,
  I960_G11 = 28,
  I960_G12 = 29,
  I960_G13 = 30,
  I960_G14 = 31,
  I960_G15 = 32,

  I960_SAT = 33,
  I960_PRCB = 34,
  I960_PC = 35,
  I960_AC = 36,
  I960_IP = 37,
  I960_PIP = 38,
};

enum {
  I960_IRQ0 = 0,
  I960_IRQ1 = 1,
  I960_IRQ2 = 2,
  I960_IRQ3 = 3,
};

void i960_get_info(UINT32 state, union cpuinfo *info);

#endif
