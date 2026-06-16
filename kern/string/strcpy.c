#include "../string.h"
#include "string.h"

char *
strcpy(char *restrict dest, const char *restrict src)
{
	for (size_t i = 0; (dest[i] = src[i]); ++i)
		;
	return dest;
}
