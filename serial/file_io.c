#include <stdlib.h>
#include <stdio.h>

/* rw = read/write 0/1 */
size_t OpenFile(FILE **fp, char *fn, size_t rw)
{
	if(rw==0)
		*fp = fopen(fn,"rb");
	else if(rw==1)
		*fp = fopen(fn,"wb");
	if(*fp==NULL)
		return 1;
	return 0;
}

