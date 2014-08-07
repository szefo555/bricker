#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


/* Pad a full line if Y is out of bounds
   Pad a full page if Z is out of bounds */
void Padding(uint8_t *dest, size_t COPY_TO, const size_t LINE) 
{
	for(size_t i=0; i<LINE; i++) {
		dest[COPY_TO] = 0;
		COPY_TO++;
	}
}

/* Copy data, pad if necessary (scanline) */
void CopyLine(uint8_t *dest, uint8_t *src, size_t COPY_TO, size_t COPY_FROM, const size_t LINE, int edge[3])
{
	printf("COPYLINE\n");
	size_t n = LINE;
	n -= edge[1];
	for(int i=edge[1]; i>0; i--) {
		dest[COPY_TO] = 0;
		COPY_TO++;
	}
	if(edge[1]==0) 
		COPY_FROM--;
	for(int i=0; i<n-edge[0]; i++) {
		dest[COPY_TO] = src[COPY_FROM];
		COPY_TO++;
		COPY_FROM++;
	}
	for(int i=n-edge[0]; i<n; i++) {
		dest[COPY_TO] = 0;
		COPY_TO++;
	}	
} 
