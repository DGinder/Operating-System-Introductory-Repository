// proc.c, 159
// all processes are coded here
// processes do not use kernel space (data.h) or code (handlers, tools, etc.)
// all must be done thru system service calls

#include "spede.h"      // cons_...() needs
#include "data.h"       // run_pid needed
#include "proc.h"       // prototypes of processes
#include "syscalls.h"   // API of system service calls
#include "tools.h"	//used for MyStrcmp

void SystemProc(void) {
	while(1) asm("inb $0x80"); // do nothing for now
}

void ShellProc(void) {
	int term, my_pid, forked_pid;
	char my_str[] = " ",
		my_msg[] = ": (GaC) Shell> ",
		get_str[100];

	while(1) {
		my_pid = GetPid();
		my_str[0] = my_pid + '0';               // id str
		term = (my_pid%2 == 1)? TERM1 : TERM2;  // which term to use

		PutStr(term, my_str);
		//cons_printf("%s\n", my_msg);

		my_msg[0] = ':'; my_msg[1] = ' '; my_msg[2] = '('; my_msg[3] = 'G'; my_msg[4] = 'a'; my_msg[5] = 'C'; my_msg[6] = ')'; my_msg[7] = ' '; my_msg[8] = 'S';
		my_msg[9] = 'h'; my_msg[10] = 'e'; my_msg[11] = 'l'; my_msg[12] = 'l'; my_msg[13] = '>'; my_msg[14] = ' ';   //resets my_msg to display correctly
		PutStr(term, my_msg);

		GetStr(term, get_str, 100); // syscall will add null

		if(MyStrcmp(get_str, "fork") == 1) { // 1 mean they're the same
			forked_pid = Fork();

		if(forked_pid == -1)
			PutStr(term, "ShellProc: cannot fork!\n\r");
		}
	}
}

