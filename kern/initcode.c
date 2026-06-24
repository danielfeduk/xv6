#include "usys.h"

int main()
{
	int n;
	if(!(n = fork())) {
		debugprnt("hello");
		exit();
	} else {
		wait();
		debugprnt("done waiting");
		while(1);
	}
}
