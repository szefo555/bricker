#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

size_t read_data(const char *fn, const size_t n[3], uint8_t *arr) {
	printf("Opening file %s Size %d %d %d\n", fn, n[0], n[1], n[2]);
	FILE *fp;	
	fp=fopen(fn,"rb");
	if(fp==NULL) {
		return 1;
	}
	for(size_t z=0; z<n[2]; z++) {
		for(size_t y=0; y<n[1]; y++) {
			for(size_t x=0; x<n[0]; x++) {
				if(fread(&arr[x+y*n[0]+z*n[1]*n[0]], sizeof(uint8_t), 1, fp) == -132) 
					return 1;
			}
		}
	}
	return fclose(fp);
}

size_t OpenFile(FILE **fp, char *fn)
{
	*fp = fopen(fn,"wb");
	if(*fp==NULL)
		return 1;
	return 0;
}

