#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "io.h"
#include "read.h"

/* 

Start with:	/bricker x y z brick_dimension^3 ghostcelldimension filename
fuel 64^3
*/

/* how many bricks are needed? */
size_t NBricks(const size_t VOLUME[3], const size_t BRICKSIZE) 
{
	size_t nobricks[3] = {0,0,0};
	if(VOLUME[0] < BRICKSIZE || VOLUME[1] < BRICKSIZE || VOLUME[2] < BRICKSIZE) 
		return 0;

	for(size_t i=0; i<3; i++) {
		nobricks[i] = VOLUME[i] / BRICKSIZE;
		if(VOLUME[i] % BRICKSIZE != 0) {
			nobricks[i]++;
		}
	}
	printf("No Bricks : %d\n", nobricks[0]*nobricks[1]*nobricks[2]);
	return nobricks[0]*nobricks[1]*nobricks[2];
}

size_t GetBytes()
{
	return 1;
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
	/* number of bricks? */
	size_t numberofbricks = NBricks(VOLUME, BRICKSIZE);
	if(numberofbricks == 0) {
		printf("ERROR determining number of bricks!\n");
		return EXIT_FAILURE;
	}
	printf("---> %d\n", numberofbricks);
	/* Array where linear data is loaded; *1 == 1byte */
	uint8_t *arr;
	arr = malloc(sizeof(uint8_t)*VOLUME[0]*VOLUME[1]*VOLUME[2]*1);

	printf("%d %d %d %d %d\n", VOLUME[0],VOLUME[1],VOLUME[2],BRICKDIM,GHOSTCELLDIM);

	if(read_data(argv[6], VOLUME, arr) != 0) {
		printf("Error read file\n");
		return EXIT_FAILURE;
	}
	
	/* offsets */ 
	size_t offset[3] = {0,0,0};
	size_t goffset[3] = {offset[0]-GHOSTCELLDIM, offset[1]-GHOSTCELLDIM, offset[2]-GHOSTCELLDIM};
	size_t orig_offset[3] = {offset[0], offset[1], offset[2]};
	
	/* get voxel size (in bytes) */
	const size_t VOXELSIZE = GetBytes();

	/* full size of bricks (data+ghostcell*2) */
	const size_t GBSIZE = BRICKSIZE+2*GHOSTCELLDIM;
	/* scanline */
	const size_t LINE = VOXELSIZE * GBSIZE;
	/* output array */
	uint8_t *bricked;
	bricked = malloc(sizeof(uint8_t)*GBSIZE*GBSIZE*GBSIZE * numberofbricks);

	/* MAIN LOOP */

	for(size_t no_b = 0; no_b<numberofbricks; no_b++) {
		offset[0] = orig_offset[0];
		offset[1] = orig_offset[1];
		offset[2] = orig_offset[2];

		/* x_edge left, y_edge top, z_edge front */
		int start[3] = {offset[0] - GHOSTCELLDIM, offset[1] - GHOSTCELLDIM, offset[2] - GHOSTCELLDIM};
		const int ORIG_START_Y = start[1];

		/* edge {x_edge left, x_edge right} */
		int x_edge[2] = {0,0};
		/* x_edge left */
		if(start[0] < 0) 
			x_edge[0] = start[0]*-1;
		/* x_edge right */
		if(offset[0]+GBSIZE-GHOSTCELLDIM >= VOLUME[0]) 
			x_edge[1] = offset[0]+GBSIZE-GHOSTCELLDIM-VOLUME[0];

		/* Z */
		for(size_t z=0; z<GBSIZE; z++) {
			/* z padding FRONT */
			if(start[2] < 0) {
				Padding(bricked, (z*GBSIZE*GBSIZE)*VOXELSIZE+no_b*GBSIZE*GBSIZE*GBSIZE, GBSIZE*GBSIZE);
				start[2] ++;
			/* z padding BACK */
			} else if (offset[2] >= VOLUME[2]) 
				Padding(bricked, (z*GBSIZE*GBSIZE)*VOXELSIZE+no_b*GBSIZE*GBSIZE*GBSIZE, GBSIZE*GBSIZE);
			else {
				/* ghostcell page */
				if(offset[2]-(GHOSTCELLDIM-z) >= 0 && z < GHOSTCELLDIM)
					offset[2] = offset[2] - GHOSTCELLDIM;

				/* Y */
				for(size_t y=0; y<GBSIZE; y++) {
					const size_t COPY_TO = (z*GBSIZE*GBSIZE+y*GBSIZE)*VOXELSIZE+no_b*GBSIZE*GBSIZE*GBSIZE;
					/* y_edge TOP */
					if(start[1] < 0) {
						Padding(bricked,COPY_TO,LINE);
						start[1]++;
					} 
					/* y_edge BOTTOM */
					else if (offset[1] >= VOLUME[1]) 
						Padding(bricked,COPY_TO,LINE);
					/* ghostcell TOP */
					else if(y<GHOSTCELLDIM) {
						const size_t COPY_FROM = (offset[0]+(offset[1]-(GHOSTCELLDIM-y))*VOLUME[0]+offset[2]*VOLUME[1]*VOLUME[0])*VOXELSIZE;
						CopyLine(bricked,arr,COPY_TO,COPY_FROM,LINE,x_edge);
					/* copy data */
					} else {
						const size_t COPY_FROM = (offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[1]*VOLUME[0])*VOXELSIZE;
						CopyLine(bricked,arr,COPY_TO,COPY_FROM,LINE,x_edge);
						offset[1]++;
					}
				} /* END OF Y LOOP */

			/* reset y; increment z */
			start[1] = ORIG_START_Y;
			offset[1] = orig_offset[1];
			offset[2]++;
			}

		} /*END OF Z LOOP */
		
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

	if(write_data(GBSIZE, numberofbricks, bricked) != 0) {
		printf("ERROR saving bricks into files\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
