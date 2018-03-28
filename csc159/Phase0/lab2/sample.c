/* sample.c ~Sample Main program for SPEDE       */

#include <spede/stdio.h>
#include <spede/flames.h>

void DisplayMsg(int i);

int main(void)
{
	int i;
	int j;

	i = 111;
	for(j=0; j<5;j++){
		DisplayMsg(i);		
		i++;
	}
	return 0;
}

void DisplayMsg(int i){
	printf("%d Hello world %d \nECS", i, 2*i);
	cons_printf("-->Hello world <--\nCPE/CSC");
}
