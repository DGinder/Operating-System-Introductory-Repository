// handlers.c, 159

#include "spede.h"
#include "types.h"
#include "data.h"
#include "tools.h"
#include "proc.h"
#include "handlers.h"
#include "syscalls.h"

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
void TimerHandler(void) {//phase 1
	int i;
	timer_ticks++;
	for(i = 0; i < PROC_NUM; i++){
		if(( pcb[i].state == SLEEPING ) && ( pcb[i].wake_time == timer_ticks)){
			pcb[i].state = RUN;
			EnQ(i, &run_q); 
		}
	}

	outportb(0x20, 0x60); //dismiss timer event (IRQ0)

	if(run_pid == 0) return;   //if the running process is SystemProc, simply return

	pcb[run_pid].run_time++; //upcount the run_time of the running process
	if(pcb[run_pid].run_time == TIME_SLICE){ //it reaches the the OS time slice
		EnQ(run_pid, &run_q); //queue it back to run queue
		run_pid = -1; //reset the running pid (to -1)  // no process running anymore
	}
}

void GetPidHandler(void){//phsae 2
    //fill out the register value in the process frame of the calling process (syscall GetPid() will subsequently pull it out to return to the calling process code)
	pcb[run_pid].proc_frame_p->EAX = run_pid;
}
void WriteHandler(proc_frame_t* p){//phase 2
	int i;
	char* strMsg  = (char *)p->ECX;

	if(p->EBX == STDOUT)  cons_printf(strMsg);
	else{
		while(*strMsg){
			outportb(p->EBX + DATA, *strMsg);
			for(i = 0; i < 5000; i++) asm("inb $0x80");
      			strMsg++;
		}
	}
}

void SleepHandler(void){//phsae 2
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

void MutexLockHandler(void){//phase 3
	if(mutex.lock == UNLOCK){
		mutex.lock = LOCK;
	}
	else{
		EnQ(run_pid, &mutex.wait_q);
		pcb[run_pid].state = WAIT;
		run_pid = 0;
	}
	return;
}
void MutexUnlockHandler(void){//phase 3
	int pid;
	if(mutex.wait_q.size == 0)
		mutex.lock = UNLOCK;
	else{
		pid = DeQ(&mutex.wait_q);
		pcb[pid].state = RUN;
		EnQ(pid, &run_q);
	}
	return;

}

void GetCharHandler(proc_frame_t *p) {  // GetChar() call, like MutexLock()
	int i;
	int fileno = p->EBX;

	if(fileno == TERM1)
		i = 0;
	else
		i = 1;

	if(terminal_buffer[i].size > 0)//if has stored input
		pcb[run_pid].proc_frame_p->ECX = DeQ(&terminal_buffer[i]); //get one from it and give it to the calling process
	else{// (input has yet arrived)
		pcb[run_pid].state = WAIT;
		EnQ(run_pid, &terminal_wait_queue[i]); //block the calling process to terminal wait queue i
		run_pid = -1;
	}
}

void PutCharHandler(int fileno) {
	int i;
	char ch = pcb[run_pid].proc_frame_p->ECX;			//ch is the ECX in proc frame (set by syscall)

	i = (fileno == TERM1)? 0 : 1;	 		//if fileno is TERM1, i is 0; otherwise 1

	outportb(fileno + DATA, ch);		//call outportb() to send ch to data register based on fileno
      	pcb[run_pid].state = WAIT;
	EnQ(run_pid, &terminal_wait_queue[i]);	//block the calling process (to terminal wait queue i, etc.)
	run_pid = -1;
}

void TermHandler(int port) {  // IRQ3 or IRQ4, like MutexUnlock()
	int i;
	char ch;
	int pid, indicator;

	//if port is TERM1, i is 0; otherwise 1
	i = (port == TERM1)? 0 : 1;

	ch = inportb(port + DATA);
	indicator = inportb(port+IIR);
      	if(indicator == IIR_RXRDY){
		if(terminal_wait_queue[i].size == 0)//if terminal wait queue is empty
			EnQ((int) ch, &terminal_buffer[i]); //append ch to terminal buffer i
		else// (some process awaits)
			pid = DeQ(&terminal_wait_queue[i]);
			pcb[pid].state = RUN;
			EnQ(pid, &run_q);

			pcb[pid].proc_frame_p->ECX = ch;
	}
	else{
		if(terminal_wait_queue[i].size > 0){
			pid = DeQ(&terminal_wait_queue[i]);
			pcb[pid].state = RUN;
			EnQ(pid, &run_q);

			pcb[pid].proc_frame_p->ECX = ch;
		}
	}
}

void ForkHandler(proc_frame_t *parent_frame_p) { // Kernel() provides this ptr
	int child_pid, delta, *bp, temp;
	proc_frame_t *child_frame_p;
	
	if(ready_q.size == 0){
		cons_printf("Kernel Panic: cannot create more process!\n");
		pcb[run_pid].proc_frame_p->EBX = -1;
		return;
	}

	child_pid = DeQ(&ready_q);				//get first process available 
	EnQ(child_pid, &run_q);					//enque it into run queue
	MyBzero((char *)&pcb[child_pid], sizeof(pcb_t));	//clear out the child_pid pcd
	pcb[child_pid].state = RUN;				//set its state to run;
	pcb[child_pid].ppid = run_pid;		//set parent pid to current pid;
	
	MyMemcpy((char *)&proc_stack[child_pid], (char *)&proc_stack[run_pid], PROC_STACK_SIZE);

	delta = proc_stack[child_pid] - proc_stack[run_pid];	//a. delta = child stack <--- byte distance ---> parent stack

	temp = ((int) parent_frame_p + delta);
	child_frame_p = (proc_frame_t *) temp;			//b. set child frame location = parent frame location + delta

	pcb[child_pid].proc_frame_p = child_frame_p;

	child_frame_p->ESP += delta;		//c. the same goes to the ESP, EBP, ESI, and EDI; in the new child process frame each of these is added with delta
	child_frame_p->EBP += delta; 		
	child_frame_p->ESI += delta;
	child_frame_p->EDI += delta;

		

	parent_frame_p->EBX = child_pid; 				//d. set the EBX in the parent's process frame to the child_pid, but it's given 0 to the EBX in the child's process frame
	child_frame_p->EBX = 0; 					 

	

	bp = (int *)child_frame_p->EBP;
	while(*bp='\0'){
       		*bp += delta;
        	bp = (int *)*bp;
	}	
}
