#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <kernel/types.h>

/** Functions. **/

void schedule();
/****************************************************************/
void schedule_save_regs(struct registers *regs, ui32_t *ebp_isr, bool_t exc);
/****************************************************************/
void schedule_switch(pid_t pid);
/****************************************************************/
void schedule_yield();

#endif
