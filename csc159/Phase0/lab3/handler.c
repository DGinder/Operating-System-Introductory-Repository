//******************************************************
// handlers.c, Phase 0, EX 4 -- Timer Event
//*****************************************************

#include "spede.h"

char my_name[] = "Darrin Ginder";
int i = 0;
int tick_count = 0;
int j;
unsigned short *char_p = (unsigned short *) 0xB8000+12*80+34;

void TimerHandler(){
	int size = (int) sizeof(my_name);
	if(tick_count == 0){
		*char_p = my_name[i] + 0xf00;
	}
	tick_count++;
	if(tick_count == 75){
		tick_count = 0;
		i++;
		char_p++;
		if(i ==  size){
			i = 0;
			char_p = (unsigned short *) 0xB8000+12*80+34;
			for(j = 0; j < size; j++){
				char_p[j] = ' ';
			} 
		}
	}
	outportb(0x20, 0x60);
}
