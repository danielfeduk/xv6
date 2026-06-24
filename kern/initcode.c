#include "usys.h"

int main()
{
	int n;
	if(!(n = fork())) {
		debugprnt("hello");
		exit(0);
	} else {
		int s;
		wait(&s);
		debugprnt("done waiting");
		debugprnt2(s);
		while(1);
	}
}
