#include <stdio.h>
#include <stdlib.h>

void fun();

void fun(){
    char* buffer;
    buffer =(char*)malloc(sizeof(char));
    buffer[0] = 'c';
    free(buffer);
    return;
}

int main(void){
    fun();
	return 0;
}  