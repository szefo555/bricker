#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

int main(int argc, char* argv[])
{
	if(argc < 5)
		return EXIT_FAILURE;

	char *endptr = NULL;
	const size_t VOLUME = strtol(argv[1], &endptr, 10);
	const size_t BRICKDIM = strtol(argv[2], &endptr, 10);
	const size_t GDIM = strtol(argv[3], &endptr, 10);

	FILE *fin;
	fin = fopen(argv[4], "rb");
	if(fin == NULL)
		return EXIT_FAILURE;

	FILE *fout;
	char fn[256];
	sprintf(fn, "rebricked_%s", argv[4]);
	fout = fopen(fn, "wb");
	if(fout == NULL)
		return EXIT_FAILURE;
	size_t cur=0;
	int a;
	uint8_t *read = malloc(sizeof(uint8_t) * BRICKDIM * BRICKDIM * BRICKDIM);
	
	for(size_t no_b = 0; no_b<8; no_b++) {


		offset[0] = orig_offset[0];
		offset[1] = orig_offset[1];
		offset[2] = orig_offset[2];

		/* Z */
		for(size_t z=0; z<GBSIZE; z++) {
			for(size_t y=0; y<GBSIZE; y++) {
				const size_t COPY_FROM = (offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[1]*VOLUME[0])*VOXELSIZE;
				CopyLine(fpo,fpi,COPY_FROM,LINE,x_edge, woffset);
				offset[1]++;
			} /* END OF Y LOOP */

			/* reset y; increment z */
			start[1] = ORIG_START_Y;
			offset[1] = orig_offset[1];
			offset[2]++;
			}

		} /*END OF Z LOOP */
	
		/* new offsets for next blocks */
		orig_offset[0] += BRICKDIM;
		if(orig_offset[0]>=VOLUME) {
			orig_offset[1] += BRICKDIM;
			if(orig_offset[1] >= VOLUME) {
				orig_offset[2] += BRICKDIM;
				orig_offset[1] = 0;
			}
		orig_offset[0] = 0;
		}
	} /* END OF no_b LOOP */
	fclose(fin);
	fclose(fout);
	return 0;
	
}
