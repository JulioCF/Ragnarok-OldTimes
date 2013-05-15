#ifndef _HARMONYCORE_H
#define _HARMONYCORE_H


#include "../common/harmserv.h"

void harmony_core_init();
void harmony_core_final();

extern struct HARMSRV_EA_FUNCS   *ea_funcs;
extern struct HARMSRV_HARM_FUNCS *harm_funcs;

#endif

