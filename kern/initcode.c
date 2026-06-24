#include "usys.h"

int main()
{
	int n;
	if(!(n = fork())) {
		debugprnt("hello");
		exit();
	}
	while(1);
}
