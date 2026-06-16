#include "../string.h"
#include <stddef.h>

size_t
strlcpy(char *d, const char *s, size_t n)
{
	char *od;

	od = d;
	if (n <= 0)
		return 0;
	while (--n > 0 && (*d++ = *s++) != 0)
		;
	*d = 0;
	return (size_t)(d - od);
}
