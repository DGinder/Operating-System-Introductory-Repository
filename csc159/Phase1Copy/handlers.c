// handlers.c, 159

#include "spede.h"
#include "types.h"
#include "data.h"
#include "tools.h"
#include "proc.h"
#include "handlers.h"

// to create process, 1st alloc PID, PCB, and process stack space
// build process frame, initialize PCB, record PID to run_q (if not 0)
void NewProcHandler(func_p_t p) {  // arg: where process code starts
   int pid;

   if(ready_q.size == 0)  {//the size of ready_q is 0 // may occur as too many processes been created
      cons_printf("Kernel Panic: cannot create more process!\n");
      return;                   // alternative: breakpoint() into GDB
   }

   pid = DeQ(&ready_q); //get a 'pid' from ready_q
   MyBzero((char *)&pcb[pid], sizeof(pcb_t)); //use tool function MyBzero to clear PCB and runtime stack
   pcb[pid].state = RUN;
   EnQ(pid, &run_q);
   //queue it (pid) to be run_q unless it's 0 (SystemProc)

   pcb[pid].proc_frame_p = (proc_frame_t *)&proc_stack[pid][0]; //point proc_frame_p to into stack (to where best to place a process frame)
   pcb[pid].proc_frame_p->EFL = EF_DEFAULT_VALUE|EF_INTR; //fill out EFL with "EF_DEFAULT_VALUE|EF_INTR" // to enable intr!
   pcb[pid].proc_frame_p->EIP = (unsigned int) p; //fill out EIP to p
   pcb[pid].proc_frame_p->CS = get_cs(); //fill CS with the return from a get_cs() call
}

// count run_time of running process and preempt it if reaching time slice
void TimerHandler(void) {
   static unsigned int timer_tick = 0; //declare a staic local (unsigned int) counter timer_tick = 0;

   if(timer_tick++ == 75){
	cons_putchar(' ');
	cons_putchar('.');	//if tick is a multiple of 75, show a period symbol on target PC
	timer_tick = 0;
   }

   outportb(0x20, 0x60); //dismiss timer event (IRQ0)

   if(run_pid == 0) return;   //if the running process is SystemProc, simply return

   pcb[run_pid].run_time++; //upcount the run_time of the running process
   if(pcb[run_pid].run_time == TIME_SLICE){ //it reaches the the OS time slice
      EnQ(run_pid, &run_q); //queue it back to run queue
      run_pid = -1; //reset the running pid (to -1)  // no process running anymore
   }
}

