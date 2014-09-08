#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "file_io.h"
#include "rw.h"
#include "multires.h"

/* how many bricks are needed? */
size_t NBricks(const size_t VOLUME[3], const size_t BRICKSIZE, size_t *bdim) 
{
	if(VOLUME[0] < BRICKSIZE || VOLUME[1] < BRICKSIZE || VOLUME[2] < BRICKSIZE) 
		return 0;

	for(size_t i=0; i<3; i++) {
		bdim[i] = VOLUME[i] / BRICKSIZE;
		if(VOLUME[i] % BRICKSIZE != 0) {
			bdim[i]++;
		}
	}
	printf("No Bricks : %d\n", bdim[0]*bdim[1]*bdim[2]);
	printf("X %d Y %d Z %d\n", bdim[0],bdim[1],bdim[2]);
	return bdim[0]*bdim[1]*bdim[2];
}

void ResetOffset(size_t o[3], size_t oo[3]) {
	for(size_t i=0; i<3; i++)
		o[i] = oo[i];
}

int main(int argc, char* argv[]) 
{
	if(argc!=7) {
		printf("Check input\n");
		return EXIT_FAILURE;
	}
	char *endptr = NULL;
	/* Size of the volume {x,y,z} */
	const size_t VOLUME[3] = { strtol(argv[1], &endptr, 10), strtol(argv[2], &endptr, 10), strtol(argv[3], &endptr, 10) };
	/* Dimension of the brick incl. ghostcell */
	const size_t BRICKDIM = strtol(argv[4], &endptr, 10);
	/* Size of ghost cells for later subtraction */	
	const size_t GHOSTCELLDIM = strtol(argv[5], &endptr, 10);
	/* subtract size of ghostcells*2 to get "real" size of bricks */
	const size_t BRICKSIZE = BRICKDIM - GHOSTCELLDIM*2;
	if(BRICKSIZE<1) {
		printf("BRICKSIZE is smaller than 1!\n");	
		return EXIT_FAILURE;
	}
	/* # bricks per dimension */
	size_t bricks_per_dimension[3];
	/* number of bricks? */
	size_t numberofbricks = NBricks(VOLUME, BRICKSIZE, bricks_per_dimension);
	if(numberofbricks == 0) {
		printf("ERROR determining number of bricks!\n");
		return EXIT_FAILURE;
	}
	
	size_t offset[3] = {0,0,0};
	size_t orig_offset[3] = {0,0,0};

	size_t LINE=BRICKSIZE+2*GHOSTCELLDIM;
	
	FILE *fpi = NULL;
	if(OpenFile(&fpi, argv[6], 0) != 0) {
		printf("Couldn't open file %s\n", argv[6]);
		return EXIT_FAILURE;	
	}

	FILE *fpo = NULL;
	char fn[256];
	sprintf(fn, "%d.raw", 1);
	if(OpenFile(&fpo, fn, 1) != 0) {
		printf("Couldn't open file %d.raw\n", 1);
		return EXIT_FAILURE;
	}
	
	for(size_t no_b=0; no_b<numberofbricks; no_b++) {
		ResetOffset(offset,orig_offset);	
		/* x_edge left, y_edge top, z_edge front */
		int start[3] = {offset[0] - GHOSTCELLDIM, offset[1] - GHOSTCELLDIM, offset[2] - GHOSTCELLDIM};
		const int ORIG_START_Y = start[1];
		/* x_edge left, x_edge right */
		int x_edge[2] = {0,0};
		/* x_edge left */
		if(start[0]<0)
			x_edge[0] = start[0]*-1;
		/* x_edge right */
		if(offset[0]+LINE-GHOSTCELLDIM >= VOLUME[0]) 
			x_edge[1] = offset[0]+LINE-GHOSTCELLDIM-VOLUME[0];


		/*Z*/
		for(size_t z=0; z<LINE; z++) {
			if(start[2]<0) {
				size_t pad[2] = {LINE*LINE,0};
				CopyData(fpi,fpo,offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],0,pad, no_b);
				start[2]++;
			} else if (offset[2] >= VOLUME[2]) {
				size_t pad[2] = {LINE*LINE,0};
				CopyData(fpi,fpo,offset[0]+offset[1]*VOLUME[0]+(offset[2]-(offset[2]-VOLUME[2])-1)*VOLUME[0]*VOLUME[1],0,pad, no_b);
			} else {
				if(z < GHOSTCELLDIM && offset[2] - (GHOSTCELLDIM-z) >= 0) {
					offset[2]-=GHOSTCELLDIM;
				}
				for(size_t y=0; y<LINE; y++) {
					/* top */
					if(start[1]<0) {
						if(offset[2]==0) {
							CopyData(fpi,fpo,offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],LINE,x_edge, no_b); start[1]++;
						} else {
							CopyData(fpi,fpo,offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1]-LINE,LINE,x_edge, no_b); start[1]++;
						}
					/* bottom */
					} else if (offset[1]>=VOLUME[1] && offset[1] < VOLUME[1]+GHOSTCELLDIM) {
						CopyData(fpi,fpo,offset[0]+(offset[1]-(offset[1]-VOLUME[1])-1)*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],LINE,x_edge, no_b);

					} else if (offset[1]>=VOLUME[1] && offset[1] >= VOLUME[1]+GHOSTCELLDIM) { 
						CopyData(fpi,fpo,offset[0]+offset[1]-(offset[1]-VOLUME[1]+GHOSTCELLDIM)*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],LINE,x_edge,no_b);	
					} else if (y<GHOSTCELLDIM) {
						CopyData(fpi,fpo,offset[0]+(offset[1]-(GHOSTCELLDIM-y))*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],LINE,x_edge, no_b);
					} else {
						CopyData(fpi,fpo,offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[0]*VOLUME[1],LINE,x_edge, no_b);	
						offset[1]++;
					}
				} /* END OF Y LOOP */
			/* reset Y, increment Z */
			start[1] = ORIG_START_Y;
			offset[1] = orig_offset[1];
			offset[2]++;
			}
		} /* END OF Z LOOP */	
		/* new offsets for next blocks */
		orig_offset[0] += BRICKSIZE;
		if(orig_offset[0]>=VOLUME[0]) {
			orig_offset[1] += BRICKSIZE;
			if(orig_offset[1] >= VOLUME[1]) {
				orig_offset[2] += BRICKSIZE;
				orig_offset[1] = 0;
			}
		orig_offset[0] = 0;
		}
	} /* END OF no_b LOOP */
	fclose(fpi);
	fclose(fpo);
	
}
