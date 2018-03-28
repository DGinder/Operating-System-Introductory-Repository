// main.c, 159
// OS bootstrap and kernel code for OS phase 1
//
// Team Name: GaC (Members: Darrin Ginder and Milton Chiem)

#include "spede.h"      // given SPEDE stuff
#include "types.h"      // data types
#include "events.h"     // events for kernel to serve
#include "tools.h"      // small functions for handlers
#include "proc.h"       // process names such as SystemProc()
#include "handlers.h"   // handler code

// kernel data are all declared here:
int run_pid;            // currently running PID; if -1, none selected
q_t ready_q, run_q;     // processes ready to be created and runables
pcb_t pcb[PROC_NUM];    // Process Control Blocks
char proc_stack[PROC_NUM][PROC_STACK_SIZE]; // process runtime stacks
unsigned int timer_ticks;

mutex_t mutex;		//phase 
int pies;

q_t terminal_buffer[2], terminal_wait_queue[2]; //phase4

q_t term_kb_wait_q[2], term_screen_wait_q[2];//phase 8

void InitTerms(int port) {
	int i;
	char str[] = "\e[2J";

// set baud, Control Format Control Register 7-E-1 (data- parity-stop bits)
// raise DTR, RTS of the serial port to start read/write

	//phase 4
	outportb(port + CFCR, CFCR_DLAB);  // CFCR_DLAB is 0x80
	outportb(port + BAUDLO, LOBYTE(115200/9600)); // period of each of 9600 bauds
	outportb(port + BAUDHI, HIBYTE(115200/9600));           
	outportb(port + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);

	outportb(port + IER, 0);//phase 5
	outportb(port + MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
	outportb(port + IER, IER_ERXRDY | IER_ETXRDY);

	for(i=0;i<LOOP;i++) asm("inb $0x80");
	
	
	outportb(port + DATA,str[0]);
	for(i = 0; i < 5000; i++) asm("inb $0x80");
	outportb(port + DATA,str[1]);
	for(i = 0; i < 5000; i++) asm("inb $0x80");
	outportb(port + DATA,str[2]);
	for(i = 0; i < 5000; i++) asm("inb $0x80");
	outportb(port + DATA,str[3]);
	for(i = 0; i < 5000; i++) asm("inb $0x80");
	outportb(port + DATA,'\r');
	for(i = 0; i < 5000; i++) asm("inb $0x80");
	
}

void ProcScheduler(void) {              // choose run_pid to load/run
	if(run_pid > 0) return; // no need if PID is a user proc

	if(run_q.size == 0)// the run_q is empty
		run_pid = 0; // let run_pid be zero
	else
		run_pid = DeQ(&run_q); //get the 1st in run_q to be run_pid

	pcb[run_pid].life_time += pcb[run_pid].run_time; //accumulate its life_time by adding its run_time
	pcb[run_pid].run_time = 0; //and then reset its run_time to zero
}

int main(void) {  // OS bootstraps
	int i;
	struct i386_gate *IDT_p; // DRAM location where IDT is
	timer_ticks = 0;
	pies = 0;
	run_pid = -1; // needs to find a runable PID

	MyBzero((char *)&ready_q, sizeof(q_t)); //use your tool function MyBzero to clear the two queues 	
	MyBzero((char *)&run_q, sizeof(q_t)); 
	MyBzero((char *)&pcb, sizeof(pcb)); 

	MyBzero((char *)&mutex, sizeof(mutex_t));//phase 3 clearing
	mutex.lock = UNLOCK; 									
		
	MyBzero((char *)&terminal_buffer, sizeof(q_t[2])); //use your tool function MyBzero to clear the two queues 	
	MyBzero((char *)&terminal_wait_queue, sizeof(q_t[2])); 

	MyBzero((char *)&term_kb_wait_q, sizeof(q_t[2])); 
	MyBzero((char *)&term_screen_wait_q, sizeof(q_t[2])); 

	InitTerms(TERM1);
	InitTerms(TERM2);

	for(i = 0; i < Q_SIZE; i++){
		EnQ(i, &ready_q);	//enqueue 0~19 to ready_q (all PID's are ready)
	}
   
	IDT_p = get_idt_base();
	cons_printf("IDT located at DRAM addr %x (%d).\n", IDT_p,IDT_p);   ///get the IDT_p (to point to/locate IDT, like in the lab exercise)
   
	fill_gate(&IDT_p[TIMER_EVENT],   (int)TimerEvent,   get_cs(), ACC_INTR_GATE, 0);
   	fill_gate(&IDT_p[SYSCALL_EVENT], (int)SyscallEvent, get_cs(), ACC_INTR_GATE, 0);
   	fill_gate(&IDT_p[TERM1_EVENT],   (int)Term1Event, get_cs(), ACC_INTR_GATE, 0);
   	fill_gate(&IDT_p[TERM2_EVENT],   (int)Term2Event, get_cs(), ACC_INTR_GATE, 0);
  	outportb(0x21, ~0x19); // IRQ 0,3,4 1101 = ~0x19

				

	NewProcHandler(SystemProc); //call NewProcHandler(SystemProc) to create the 1st process
	ProcScheduler();  //call ProcScheduler() to select the run_pid
	ProcLoader(pcb[run_pid].proc_frame_p); // call ProcLoader with the proc_frame_p of the selected run_pid

	return 0; // compiler needs for syntax altho this statement is never exec
}

void Kernel(proc_frame_t *proc_frame_p) {   // kernel code runs (100 times/second)
	char key;
	pcb[run_pid].proc_frame_p = proc_frame_p;   //save the proc_frame_p to the PCB of run_pid			
                      

	switch(proc_frame_p->event_type ){
		case TIMER_EVENT:
			TimerHandler();
			break;
	     	
		case SYSCALL_EVENT:
			switch( pcb[run_pid].proc_frame_p->EAX ){
				case WRITE:
					WriteHandler(proc_frame_p);
					break;
				case GETPID:
					GetPidHandler();
					break;
				case SLEEP:
					SleepHandler();
					break;
				case MUTEX:
			    		switch(proc_frame_p->EBX){
						case LOCK:
							MutexLockHandler();
							break;
						case UNLOCK:
							MutexUnlockHandler();
							break;
						default:
							break;
					}
					break;
				case GETCHAR:
					GetCharHandler(pcb[run_pid].proc_frame_p);
					break;
				case PUTCHAR:
					PutCharHandler(pcb[run_pid].proc_frame_p->EBX);
					break;
				case FORK:
					ForkHandler(proc_frame_p);
					break;
				case SIGNAL:
					SignalHandler(proc_frame_p);
					break;
				case EXIT:
					ExitHandler(proc_frame_p);
					break;
				case WAITPID:
					WaitPidHandler(proc_frame_p);
					break;
			}
			break;

		case TERM1_EVENT:
			TermHandler(TERM1);
			outportb(0x20, 0x63);
			break;
		case TERM2_EVENT:
			TermHandler(TERM2);
			outportb(0x20, 0x64);
			break;	
				
		default:
		break;
	}

	if(cons_kbhit()!= 0){ //if a key has been pressed on Target PC {
		key = cons_getchar();     //get the key
		if(key == 'n') NewProcHandler(ShellProc); //if it's 'n,' create a new ShellProc by calling NewProcHandler()
		if(key == 'b') breakpoint(); //if it's 'b,' go to the GDB prompt, by calling breakpoint()
	}
	ProcScheduler(); //call ProcScheduler() to select run_pid (if needed)
	ProcLoader(pcb[run_pid].proc_frame_p); //given the proc_frame_p of the run_pid
}



