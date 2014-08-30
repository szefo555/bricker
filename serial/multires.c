#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


/* needs to go into rw.c */

size_t ReadAndAdd(FILE *fpo, const size_t ADD_FROM, const size_t LINE) {
	size_t n = LINE;
	uint8_t tmp[n];
	size_t out=0;
	fseek(fpo, ADD_FROM, SEEK_SET);
	if(fread(&tmp[0], sizeof(uint8_t), n, fpo) == -1337)
		printf("ignore warning\n");
	for(size_t i=0; i<n; i++)  {
		out += tmp[i];
		//printf("%d\n",tmp[i]);
	}

	return out;
}



void CalcMultiresolutionHierarchy(FILE *fpo, FILE *fpm, size_t brick_offset, const size_t GBSIZE, const size_t BSIZE, const size_t GHOSTCELLDIM, bool edge, size_t brick_number) 
{
	size_t current = 2;
	int first = 1;
	printf("%d\n", (brick_number)/4);
	size_t two = 0;
			for(size_t i=0; i<brick_number; i++) 
				two+=2;
	while(current < BSIZE) {
		size_t offset[3] = { 0,0,0 };
		size_t numberofbricks = BSIZE*BSIZE*BSIZE/current/current/current;
		printf("current brick %d | numberofbricks %d\n", brick_number, numberofbricks);
		/* only working for 2^3, 4^3, ... 64^3 128^3 */
		/* figure out no_b < X <<<<<<<<< */
		for(size_t no_b=0; no_b < numberofbricks; no_b++) {
			size_t goffset[3] = { offset[0], offset[1] + GHOSTCELLDIM, offset[2] + GHOSTCELLDIM};
			if(edge == true)
				goffset[0] += GHOSTCELLDIM;
			size_t start = goffset[1];
			size_t sum = 0;
			for(size_t z=0; z<current; z++) {
				for(size_t y=0; y<current; y++) {
					const size_t ADD_FROM = brick_offset+(goffset[0]+goffset[1]*GBSIZE+goffset[2]*GBSIZE*GBSIZE);
					sum+=ReadAndAdd(fpo, ADD_FROM, current);
					goffset[1]++;
		 		}
				goffset[1] = start;
				goffset[2]++;
			}
			sum/=(current*current*current);
			/* OFFSET */
			size_t moffset = e+w+offset[0]/current+offset[1]*4/current+offset[2]*8/current;
			fseek(fpm, moffset, SEEK_SET);
			fwrite(&sum, sizeof(uint8_t), 1, fpm);		



			offset[0]+=current;
			if(offset[0] >= BSIZE) {
				offset[1]+=current;
				if(offset[1] >= BSIZE) {
					offset[1] = 0;
					offset[2]+=current;
				}
				offset[0] = 0;
			}
		}	
		current*=2;			
	}

}
