#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <mpi.h>
#include "file_io.h"
#include "rw.h"

/* Start with:	mpiexec -n CORES /bricker x y z brick_dimension^3 ghostcelldimension filename
 fuel 64^3
*/

/* Calculate how many pages each rank will brick. 
 page = x*y */
void getOffsets(size_t *o, size_t size, size_t z, size_t bs) 
{
	size_t offset = z/size;
	size_t mod = z%size;
	for(size_t i=0; i<size; i++) {
		o[i] = offset;
		if(mod!=0) {
			o[i]++;	
			mod--;
		}
	}
}

void getBricks(size_t *b, size_t size, size_t nb) 
{
	size_t mybricks = nb/size;
	size_t mod = nb%size;
	printf("sdsds %d\n", mod);
	for(size_t i=0; i<size; i++) {
		b[i] = mybricks;
		if(mod!=0) {
			b[i]*=2;
			mod--;
		}
	}
}	

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
	//printf("No Bricks : %d\n", nobricks[0]*nobricks[1]*nobricks[2]);
	return nobricks[0]*nobricks[1]*nobricks[2];
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
	/* number of bricks? */
	size_t numberofbricks = NBricks(VOLUME, BRICKSIZE);
	if(numberofbricks == 0) {
		printf("ERROR determining number of bricks!\n");
		return EXIT_FAILURE;
	}
	printf("SSSSSSSSSSSSSSS %d\n", numberofbricks);
	size_t myoffset;
	size_t mybricks;
	size_t offsets[size];
	size_t bricks[size];

	if(rank==0) {
		getOffsets(offsets, size, VOLUME[2], BRICKSIZE);
		myoffset=0;
		getBricks(bricks, size, numberofbricks);
		mybricks=bricks[0];
		size_t o=0;
		for(size_t i=1; i<size; i++) {
			o+=offsets[i];
			if(MPI_Send(&o, 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != 0) {
				fprintf(stderr, "ERROR sending offset\n");
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
			if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != 0) {
				fprintf(stderr, "ERROR sending # of blocks\n");
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
		}
	} else {
		if(MPI_Recv(&myoffset, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			fprintf(stderr, "ERROR receiving offset\n");
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
		}
			if(MPI_Recv(&mybricks, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != 0) {
			fprintf(stderr, "ERROR receiving # of blocks\n");
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
		}
	}
	printf("%d: %d %d\n", rank, myoffset, mybricks);
	
	/* local offsets for every rank */
	size_t offset[3] = {0,0,myoffset};
	size_t goffset[3] = {offset[0]-GHOSTCELLDIM,offset[1]-GHOSTCELLDIM,offset[2]-GHOSTCELLDIM};	
	size_t orig_offset[3] = {offset[0],offset[1],offset[2]};

	/* voxel size in bytes */
	const size_t VOXELSIZE = GetBytes();

	/* full size of bricks (data+ghostcell*2) */	
	const size_t GBSIZE = BRICKSIZE+2*GHOSTCELLDIM;
	/* scanline */
	const size_t LINE = VOXELSIZE * GBSIZE;

	/* input file stream */
	FILE *fpi;
	if(OpenFile(&fpi, argv[6], 0) != 0) {
		printf("Rank %d couldn't open file %s\n", rank, argv[6]);	
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	/* output file stream */	
	FILE *fpo;
	char fn[32];	
	sprintf(fn, "%d.raw", rank);
	if(OpenFile(&fpo, fn, 1) != 0) {
		printf("Rank %d couldn't open file %s\n", rank, fn);	
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}

	/* MAIN LOOP */
	

	for(size_t no_b=0; no_b<mybricks; no_b++) {
	
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
			/* z padding front */
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
	
	MPI_Finalize();		
	
	return EXIT_SUCCESS;
}
