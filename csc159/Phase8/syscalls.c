// syscalls.c
// API calls to kernel system service

#include "types.h"

int GetPid(void) {          // no input, has return
	int pid;

	asm("
		movl $100, %%EAX;   // service #100
		int $128;           // interrupt CPU with IDT Event 128
		movl %%EAX, %0"       // restore original EAX
			: "=g" (pid)         // output syntax, for output argument
			:                    // no input items
			: "eax"
	);
	return pid;
}

// Write() call
void Write(int fileno, char *p) {
	asm("//   save registers that will be used here
		movl $4, %%EAX;
		movl %0, %%EBX;
		movl %1, %%ECX;	//   send in service #, fileno, and p via three suitable registers
		int $128"
			:          // no outputs, otherwise, use "=g" (...)
			: "g" (fileno), "g" ((int)p)  // inputs, %0 and %1
			:"eax", "ebx", "ecx"
	);
}

void Sleep(int i) {
	asm("
		movl $101, %%EAX;
		movl %0, %%EBX;// send in # of seconds to sleep (in EBX)
		int $128"
		// 101 -> EAX, call "int 128", 
			:
			: "g" (i)
			:"eax", "ebx"
	);
}
void Mutex(int i){
	asm("
		movl $102, %%EAX;  //move mutex syscall #
		movl %0, %%EBX;   //move arguement
		int $128"
			:
			:"g" (i)
			: "eax", "ebx"
	);
}

//phase5
char GetChar(int fileno){
	int ch;
	asm("
		movl $103, %%EAX;
		movl %1, %%EBX;
		int $128;
		movl %%ECX, %0"
		
			: "=g" (ch)
			: "g" (fileno)
			: "eax", "ebx", "ecx"
	);
	return (char) ch;
			
}

//phase6
void PutChar(int fileno, char ch) {
	asm("
		movl $104, %%EAX;
		movl %0, %%EBX;
		movl %1, %%ECX;
		int $128"
		
			:
			: "g" (fileno), "g" ((int) ch)
			: "eax", "ebx", "ecx"
	);
}

void PutStr(int fileno, char *p) {
	while(*p){	//loop until the string 'p' points to ends
		PutChar(fileno, *p); //call PutChar() with 'fileno' and a char (has to do with 'p')
      		p++;	//advance 'p'
	}
}

void GetStr(int fileno, char *p, int size) {
	char ch = ' ';
	char* temp = p;
	while(size > 1){	//loop to call GetChar() to get a character
								//until it's a RETURN (ASCII 13 -- CR) or ENTER (ASCII 10 -- NL) key,
		ch = (char) GetChar(fileno);		//to get it into the string (at the location where? has to do with 'p')
		PutChar(fileno, ch);
		if(ch == '\r') PutChar(fileno, '\n');	
		if(ch == (char)13 || ch == (char)10){
			break;
		}//print character gotten
		*temp = (int) ch;
		temp++;
		
		size--;				//or when 'size' is getting too small
	}
	
	*temp = (char)0;
	return;
	
}

//phase7
int Fork(void){
	int child_pid;
	asm("
		movl $2, %%EAX;
		int $128;
		movl %%EBX, %0"
			: "=g" (child_pid)
			:
			: "eax", "ebx"
	);
	return child_pid;
}

//phase8
void Signal(int sig, func_p_t fun){
	asm("
		movl $48, %%EAX;
		movl %0, %%EBX;
		movl %1, %%ECX;
		int $128"
			:
			: "g" (sig), "g" (fun)
			:"eax", "ebx", "ecx"
	);
}
