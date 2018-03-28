//**************************************
//events.S, Phase 0, EX 4 -- Timer Event
//************************************


#include <spede/machine/asmacros.h>
#include "events.h"

ENTRY(TimerEvent)
	pusha
	cld
	call CNAME(TimerHandler)
	popa
	iret
