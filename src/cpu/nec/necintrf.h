/* ASG 971222 -- rewrote this interface */
#ifndef __NEC_H_
#define __NEC_H_

#include "memory.h"
#include "osd_cpu.h"

enum {
	NEC_IP=1, NEC_AW, NEC_CW, NEC_DW, NEC_BW, NEC_SP, NEC_BP, NEC_IX, NEC_IY,
	NEC_FLAGS, NEC_ES, NEC_CS, NEC_SS, NEC_DS,
	NEC_VECTOR, NEC_PENDING };

/* Public functions */
extern void v20_get_info(UINT32 state, union cpuinfo *info);
extern void v30_get_info(UINT32 state, union cpuinfo *info);
extern void v33_get_info(UINT32 state, union cpuinfo *info);

#ifdef MAME_DEBUG
extern unsigned Dasmnec(char* buffer, unsigned pc);
#endif

#endif
