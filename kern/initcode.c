#include "usys.h"

int main()
{
	int n;
	int pfd[2] = {0};
	if(pipe(pfd) == -1)
		debugprnt("pipefail");

	char a[] = " world!";
	char b[sizeof(a)];

	if(write(pfd[1], sizeof(a), a) == -1)
		debugprnt("writefail");

	if(!(n = fork())) {
		struct stat x;
		debugprnt("hello");
		if(read(pfd[0], sizeof(a), b) == -1)
			debugprnt("readfail");
		if(fstat(pfd[0], &x) == -1)
			debugprnt("statfail (this should happen)");
		debugprnt(b);
		exit(0);
	} else {
		int s;
		wait(&s);
		debugprnt("done waiting");
		debugprnt2(s);
		while(1);
	}
}
