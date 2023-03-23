#include<stdio.h>
#include "mymath.h"

int main(int argc, char *argv[]){

	int a= 12, b = 4;
	
	printf("%d+%d=%d\n",a, b , add(a, b));
	
	printf("%d-%d=%d\n",a, b , sub(a, b));
	
	printf("%d/%d=%d\n",a, b , div(a, b));

	return 0;
}
