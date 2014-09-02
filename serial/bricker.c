#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "file_io.h"
#include "rw.h"
#include "multires.h"

/* 
Start with:	/bricker x y z brick_dimension^3 ghostcelldimension filename
fuel 64^3
*/

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
	/* # bricks per dimension */
	size_t bricks_per_dimension[3];
	/* number of bricks? */
	size_t numberofbricks = NBricks(VOLUME, BRICKSIZE, bricks_per_dimension);
	if(numberofbricks == 0) {
		printf("ERROR determining number of bricks!\n");
		return EXIT_FAILURE;
	}
	/* Array where linear data is loaded; *1 == 1byte */
	printf("%d %d %d %d %d\n", VOLUME[0],VOLUME[1],VOLUME[2],BRICKDIM,GHOSTCELLDIM);

	
	/* offsets */ 
	size_t offset[3] = {0,0,0};
	size_t orig_offset[3] = {offset[0], offset[1], offset[2]};
	
	/* get voxel size (in bytes) */
	const size_t VOXELSIZE = GetBytes();

	/* full size of bricks (data+ghostcell*2) */
	const size_t GBSIZE = BRICKSIZE+2*GHOSTCELLDIM;
	/* scanline */
	const size_t LINE = VOXELSIZE * GBSIZE;

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
				Padding(fpo,GBSIZE*GBSIZE);
				start[2] ++;
			/* z padding BACK */
			} else if (offset[2] >= VOLUME[2]) 
				Padding(fpo,GBSIZE*GBSIZE);
			else {
				/* ghostcell page */
				if(offset[2]-(GHOSTCELLDIM-z) >= 0 && z < GHOSTCELLDIM)
					offset[2] = offset[2] - GHOSTCELLDIM;

				/* Y */	
				for(size_t y=0; y<GBSIZE; y++) {
					/* y_edge TOP */
					if(start[1] < 0) {
						Padding(fpo,LINE);
						start[1]++;
					} 
					/* y_edge BOTTOM */
					else if (offset[1] >= VOLUME[1]) 
						Padding(fpo,LINE);
					/* ghostcell TOP */
					else if(y<GHOSTCELLDIM) {
						const size_t COPY_FROM = (offset[0]+(offset[1]-(GHOSTCELLDIM-y))*VOLUME[0]+offset[2]*VOLUME[1]*VOLUME[0])*VOXELSIZE;
						CopyLine(fpo,fpi,COPY_FROM,LINE,x_edge);
					/* copy data */
					} else {
						const size_t COPY_FROM = (offset[0]+offset[1]*VOLUME[0]+offset[2]*VOLUME[1]*VOLUME[0])*VOXELSIZE;
						CopyLine(fpo,fpi,COPY_FROM,LINE,x_edge);
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
	fclose(fpi);
	fclose(fpo);

	if(OpenFile(&fpo, "1.raw", 0) != 0) {
		printf("Couldn't open file %d.raw\n", 1);
		return EXIT_FAILURE;
	}

	/* file for multiresolution hierarchy */
	FILE *fpm; 	
	if(OpenFile(&fpm, "multires.raw", 1) != 0) {
		printf("Couldn't open file %d.raw\n", 1);
		return EXIT_FAILURE;
	}

	const size_t BRICKCUBED = GBSIZE*GBSIZE*GBSIZE;
	size_t leftside = 1;

	for(size_t no_b = 0; no_b < numberofbricks; no_b++) {
		if(leftside>=bricks_per_dimension[0]) {
			leftside=0;
			CalcMultiresolutionHierarchy(fpo, fpm, no_b*BRICKCUBED, GBSIZE, BRICKSIZE, GHOSTCELLDIM, true, no_b, bricks_per_dimension);
		} else	
			CalcMultiresolutionHierarchy(fpo, fpm, no_b*BRICKCUBED, GBSIZE, BRICKSIZE, GHOSTCELLDIM, false, no_b, bricks_per_dimension);

		leftside++;
	}
	printf("X %d Y %d Z %d\n", bricks_per_dimension[0],bricks_per_dimension[1],bricks_per_dimension[2]);

	fclose(fpm);
	fclose(fpo);	
	return EXIT_SUCCESS;
}
