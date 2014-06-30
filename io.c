#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

size_t read_data(const char *fn, size_t n, uint8_t *arr) {
	printf("Opening file %s Size %d^3\n", fn, n);
	FILE *fp;	
	fp=fopen(fn,"rb");
	if(fp==NULL) {
		return 1;
	}
	for(size_t z=0; z<n; z++) {
		for(size_t y=0; y<n; y++) {
			for(size_t x=0; x<n; x++) {
				if(fread(&arr[x+y*n+z*n*n], sizeof(uint8_t), 1, fp) == -132) return 1;
			}
		}
	}
	return fclose(fp);
}

size_t write_data(size_t n, size_t bricks, uint8_t *arr) {
	FILE *fp;
	size_t name = 1;
	for(size_t no_b=0; no_b<bricks; no_b++) {
		char fn[256];
		sprintf(fn, "%d.raw", name);
		fp=fopen(fn,"wb");
		if(fp==NULL) {
			return 1;
		}
		for(size_t z=0; z<n;z++){
			for(size_t y=0;y<n;y++){
				for(size_t x=0;x<n;x++){
					size_t index = no_b*n*n*n+z*n*n+y*n+x;
					//if(no_b>10) printf("%d\n",index);
					fwrite(&arr[index], sizeof(uint8_t), 1, fp);
				}
			}
		}
	if(fclose(fp) != 0) return 1;
	printf("Brick %d written\n", name);
	name++;
	}
	return 0;
}
