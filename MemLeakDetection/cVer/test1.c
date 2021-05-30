#include <stdio.h>
#include <stdlib.h>

void fun();

void fun(){
    char* buffer;
    buffer =(char*) malloc(sizeof(char) * 10);
    buffer[0] = 'c';
    return;
}

int main(void){
    fun();
	return 0;
}  