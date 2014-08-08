#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


/* Pad a full line if Y is out of bounds
   Pad a full page if Z is out of bounds */
void Padding(FILE *fp, const size_t LINE) 
{
	size_t zero = 0;
	for(size_t i=0; i<LINE; i++) {
		fwrite(&zero, sizeof(uint8_t), 1, fp);
	}
}

/* Copy data, pad if necessary (scanline) */
void CopyLine(FILE *fpo, FILE *fpi, size_t COPY_FROM, const size_t LINE, int edge[3])
{
	size_t zero = 0;
	size_t n = LINE;
	n -= edge[1];
	uint8_t tmp[1];
	for(int i=edge[1]; i>0; i--) {
		fwrite(&zero, sizeof(uint8_t), 1, fpo);
	}
	if(edge[1]==0) 
		COPY_FROM--;
	fseek(fpi, +COPY_FROM, SEEK_SET);
	printf("%d\n", COPY_FROM);
	for(int i=0; i<n-edge[0]; i++) {
		fread(&tmp[0], sizeof(uint8_t), 1, fpi);
		printf("%d\n", tmp[0]);
		fwrite(&tmp[0], sizeof(uint8_t), 1, fpo);
			
	}
	for(int i=n-edge[0]; i<n; i++) {
		fwrite(&zero, sizeof(uint8_t), 1, fpo);
	}	
} 
