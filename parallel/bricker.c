#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <mpi.h>
#include <stdbool.h>
#include "file_io.h"
#include "rw.h"
#include "multires.h"

/* Start with:	mpiexec -n CORES /bricker x y z brick_dimension^3 ghostcelldimension filename
 fuel 64^3
*/

void getOffsets(size_t o[][3], size_t *b, const size_t VOLUME[3], const size_t BS, size_t size) 
{
	o[0][0]=0;
	o[0][1]=0;
	o[0][2]=0;
	for(size_t i=1; i<size; i++) {
		
		o[i][0]=o[i-1][0];
		o[i][1]=o[i-1][1];
		o[i][2]=o[i-1][2];
		size_t no_b = b[i-1];

		while(no_b > 0) {
			o[i][0] += BS;
			if(o[i][0]>=VOLUME[0]) {
				o[i][1] += BS;
				if(o[i][1] >= VOLUME[1]) {
					o[i][2] += BS;
					o[i][1] = 0;
				}
				o[i][0] = 0;
			}
			no_b--;
		}
	}
}

void getBricks(size_t *b, size_t *sb, size_t size, size_t nb) 
{
	size_t mybricks = nb/size;
	size_t mod = nb%size;
	for(size_t i=0; i<size; i++) {
		b[i] = mybricks;
		if(mod!=0) {
			b[i]++;
			mod--;
		}
		sb[i] = 0;
		if(i>0)
			sb[i] += b[i-1] + sb[i-1];
	}
}	

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

int main(int argc, char **argv) 
{
	MPI_Init(&argc,&argv);
	int size,rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);	

	if(argc!=7) {
		printf("Check input\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
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
	
	size_t myoffsets[3];
	size_t mybricks;
	size_t offsets[size][3];
	size_t bricks[size];
	size_t starting_brick[size];
	size_t mystart;

	if(rank==0) {
		getBricks(bricks, starting_brick, size, numberofbricks);
		mybricks=bricks[0];
		mystart=starting_brick[0];
		getOffsets(offsets, bricks, VOLUME, BRICKSIZE, size);
		myoffsets[0] = 0;
		myoffsets[1] = 0;
		myoffsets[2] = 0;
		if(size > 1) {
			for(size_t i=1; i<size; i++) {
				if(MPI_Send(&offsets[i][0], 3, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != 0) {
					fprintf(stderr, "ERROR sending offset\n");
					MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
				if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != 0) {
					fprintf(stderr, "ERROR sending # of blocks\n");
					MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
				if(MPI_Send(&starting_brick[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != 0) {
					fprintf(stderr, "ERROR sending starting brick\n");
					MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
			}
		}
	} else {
		if(MPI_Recv(&myoffsets[0], 3, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			fprintf(stderr, "ERROR receiving offset\n");
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
		}
		if(MPI_Recv(&mybricks, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			fprintf(stderr, "ERROR receiving # of blocks\n");
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
		}
		if(MPI_Recv(&mystart, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			fprintf(stderr, "ERROR receiving starting brick\n");
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
		}
	}
	printf("RANK: %d BRICKS: %d OFFSET_X %d OFFSET_Y %d OFFSET_Z %d\n", rank, mybricks, myoffsets[0], myoffsets[1], myoffsets[2]);
	/* local offsets for every rank */
	size_t offset[3] = {myoffsets[0],myoffsets[1],myoffsets[2]};
	size_t orig_offset[3] = {offset[0],offset[1],offset[2]};

	/* voxel size in bytes */
	const size_t VOXELSIZE = GetBytes();

	/* full size of bricks (data+ghostcell*2) */	
	const size_t GBSIZE = BRICKSIZE+2*GHOSTCELLDIM;
	/* scanline */
	const size_t LINE = VOXELSIZE * GBSIZE;

	/* input file stream */
	MPI_File fpi;
	//OpenFile(fpi, argv[6], 0);
	MPI_File_open(MPI_COMM_WORLD, argv[6], MPI_MODE_RDONLY, MPI_INFO_NULL, &fpi);
	
	
	uint8_t tmp;

	MPI_File_read(fpi, &tmp, 1, MPI_UINT8_T, MPI_STATUS_IGNORE);
	printf("%d\n",tmp);
	/* output file stream */	
	MPI_File fpo;
	char fn[256];	
	sprintf(fn, "b_%d_%d_%d_%d^3.raw", VOLUME[0], VOLUME[1], VOLUME[2], GBSIZE);
	MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fpo);
	
	/* set file pointer in output file */
	size_t goff[3] = {0,0,0};
	for(size_t i=0; i<mystart; i++) {
		goff[0]++;
		if(goff[0] >= bricks_per_dimension[0]) {
			goff[1]++;
			goff[0]=0;
			if(goff[1] >= bricks_per_dimension[1]) {
				goff[2]++;	
				goff[1]=0;
			}
		}
	}

	/* MAIN LOOP */
	for(size_t no_b = mystart; no_b<mystart+mybricks; no_b++) {

		int woffset = no_b*GBSIZE*GBSIZE*GBSIZE;
		MPI_File_seek(fpo, woffset, MPI_SEEK_SET);

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
						const size_t WRITE_AT = 0;
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
	printf("RANK %d has just finished bricking\n", rank);
    	MPI_Barrier(MPI_COMM_WORLD);
	MPI_File_close(&fpi);

	/* file for multiresolution hierarchy */
	MPI_File fpm;	
	char mfn[256];	
	sprintf(mfn, "m_%d_%d_%d_%d^3.raw", VOLUME[0], VOLUME[1], VOLUME[2], GBSIZE);
	MPI_File_open(MPI_COMM_WORLD, mfn, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fpm);

	/* MULTIRES */
	const size_t BRICKCUBED = GBSIZE*GBSIZE*GBSIZE;
	size_t leftside = 1;

	for(size_t no_b = mystart; no_b < mystart+mybricks; no_b++) {
		if(leftside>=bricks_per_dimension[0]) {
			leftside=0;
			CalcMultiresolutionHierarchy(fpo, fpm, no_b*BRICKCUBED, GBSIZE, BRICKSIZE, GHOSTCELLDIM, true, no_b, bricks_per_dimension, rank);
		} else	
			CalcMultiresolutionHierarchy(fpo, fpm, no_b*BRICKCUBED, GBSIZE, BRICKSIZE, GHOSTCELLDIM, false, no_b, bricks_per_dimension, rank);

		leftside++;
	}
	MPI_File_close(&fpo);
	MPI_File_close(&fpm);
	printf("Rank %d has multiresolution finished. Starting block was %d\n", rank, mystart);

	/* END OF MULTIRES */


	MPI_Finalize();			
	return EXIT_SUCCESS;
}
