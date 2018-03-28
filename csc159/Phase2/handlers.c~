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
   MyBzero( (char *)&proc_stack[pid], sizeof(proc_stack[pid]));
   pcb[pid].state = RUN;
   if( pid != 0 ) 
	EnQ( pid, &run_q );
   //queue it (pid) to be run_q unless it's 0 (SystemProc)

   pcb[pid].proc_frame_p = (proc_frame_t *)&proc_stack[pid][PROC_STACK_SIZE-sizeof(proc_frame_t)]; //point proc_frame_p to into stack (to where best to place a process frame)
   pcb[pid].proc_frame_p->EFL = EF_DEFAULT_VALUE|EF_INTR; //fill out EFL with "EF_DEFAULT_VALUE|EF_INTR" // to enable intr!
   pcb[pid].proc_frame_p->EIP = (unsigned int) p; //fill out EIP to p
   pcb[pid].proc_frame_p->CS = get_cs(); //fill CS with the return from a get_cs() call
}

// count run_time of running process and preempt it if reaching time slice
void TimerHandler(void) {
   int i;
   timer_ticks++;
   for(i = 0; i < PROC_NUM; i++){
      if(( pcb[ i ].state == SLEEPING ) && ( pcb[ i ].wake_time < timer_ticks)){
         
         pcb[i].state = RUN;
         EnQ(i, &run_q); 
      }
   }

   outportb(0x20, 0x60); //dismiss timer event (IRQ0)

   if(run_pid == 0) return;   //if the running process is SystemProc, simply return

   pcb[run_pid].run_time++; //upcount the run_time of the running process
   if(pcb[run_pid].run_time == TIME_SLICE){ //it reaches the the OS time slice
      if( pcb[ run_pid ].state == READY )
	pcb[ run_pid ].state= RUN;
      
      else if( pcb[ run_pid ].state == RUN )
	pcb[ run_pid ].state = READY;

      EnQ(run_pid, &run_q); //queue it back to run queue
      run_pid = -1; //reset the running pid (to -1)  // no process running anymore
   }
}

void GetPidHandler(void){
    //fill out the register value in the process frame of the calling process (syscall GetPid() will subsequently pull it out to return to the calling process code)
   pcb[run_pid].proc_frame_p->EAX = run_pid;
}
void WriteHandler(void){
  //cons_printf("%c", (char *)pcb[run_pid].proc_frame_p->ECX);
  if(pcb[run_pid].proc_frame_p->EBX == STDOUT){
        cons_printf("%s", (char *)pcb[ run_pid ].proc_frame_p->ECX);
  }
  else
	cons_printf("\r\nWriteHandler Action Error");
}

void SleepHandler(void){
      //(syscall Sleep() has placed the sleep time in a CPU register)
      //calculate the wake time by the current time plus the sleep time
      //(both in terms of timer-event counts), and place it into the
      //PCB of the calling process, alter its state, reset the running PID
   pcb[ run_pid ].wake_time = timer_ticks + 100 * pcb[ run_pid ].proc_frame_p->EBX;
    //Change state and wait until wake_time
   pcb[ run_pid ].state = SLEEPING;
    //Return CPU usage to SysProc
   run_pid = -1;
}


