#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "brick.h"
#include "lod.h"
#include "send_settings.h"
#include "misc.h"

int main(int argc, char* argv[])
{
	double init_time, brick_time, lod_time;
	double start_time = 0;
	MPI_Init(&argc,&argv);
	int size,rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if(rank==0) {
		start_time = MPI_Wtime();
	}	
	if(argc!=7 || strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0) {
		if(rank==0)
			PrintHelp();
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	
	}
	char *endptr = NULL;
	/* Size of the volume {x,y,z} */
	size_t *VOLUME = malloc(sizeof(size_t)*3);
	VOLUME[0]=(size_t)strtol(argv[1], &endptr, 10);
	VOLUME[1]=(size_t)strtol(argv[2], &endptr, 10);
	VOLUME[2]=(size_t)strtol(argv[3], &endptr, 10);
	/* Dimension of the brick excl. Ghostcells */
	const size_t BRICKSIZE = (size_t)strtol(argv[4], &endptr, 10);
	if(BRICKSIZE<1) {
		printf("BRICKSIZE is smaller than 1!\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	/* Size of ghost cells for later subtraction */
	const size_t GHOSTCELLDIM = (size_t)strtol(argv[5], &endptr, 10);

	/* # bricks per dimension */
	size_t bricks_per_dimension[3];
	/* number of bricks? */
	size_t numberofbricks = NBricks(VOLUME, BRICKSIZE, bricks_per_dimension);
	if(numberofbricks == 0) {
		printf("ERROR determining number of bricks!\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}

	/* INITIALIZATION */
	size_t *myoffsets = malloc(sizeof(size_t)*3);
	size_t mybricks;
	size_t bricks[size];
	size_t starting_brick[size];
	size_t mystart;
	size_t GBSIZE=BRICKSIZE+2*GHOSTCELLDIM;

	if(Init_MPI(rank, size, &mybricks, &mystart, myoffsets, bricks, starting_brick, numberofbricks, VOLUME, BRICKSIZE) != 0) {
		printf("ERROR @ Init_MPI\n");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	if(rank==0) {
		init_time = MPI_Wtime();
		init_time -= start_time;
		start_time = MPI_Wtime();
	}
	/* BRICKING START */
	/* input file stream */
	MPI_File fpi;
	int err;
	err = MPI_File_open(MPI_COMM_WORLD, argv[6], MPI_MODE_RDONLY, MPI_INFO_NULL, &fpi);
	if(err) {
		printf("ERROR opening file %s\n", argv[6]);
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}

	/* output file stream */
	MPI_File fpo;
	char fn[256];
	sprintf(fn, "b_%zu_%zu_%zu_%zu^3.raw", VOLUME[0], VOLUME[1], VOLUME[2], GBSIZE);
	err = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fpo);
	if(err) {
		printf("ERROR opening file %s\n", fn);
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}

	/* read from input file, reorganize data and write into output file */
	if(brick(fpi, fpo, mystart, mybricks, GBSIZE, GHOSTCELLDIM, BRICKSIZE, VOLUME, bricks_per_dimension) != 0) {
		printf("ERROR @ Rank %d: brick() failed \n", rank);
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	}
	MPI_File_close(&fpi);
	MPI_File_close(&fpo);
	if(rank==0) {
		brick_time = MPI_Wtime();
		brick_time -= start_time;
		start_time = MPI_Wtime();
	}
	/* END OF BRICKING */

	size_t lod = 1;
	/* TODO set finished correct */
	bool finished = false;
	while(!finished) {
		/* read from */
		MPI_File fin;
		MPI_File fout;
		if(lod==1) {
			err = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
			if(err) {
				printf("Rank %d: ERROR opening file %s\n", rank, fn);
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
		} else {
			char filename_read[256];
			sprintf(filename_read, "xmulti_%zu.raw", lod-1);
			err = MPI_File_open(MPI_COMM_WORLD, filename_read, MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
			if(err) {
				printf("Rank %d: ERROR opening file %s\n", rank, fn);
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			}
		}
		/* write to */
		char filename_write[256];
		sprintf(filename_write, "xmulti_%zu.raw", lod);
		err = MPI_File_open(MPI_COMM_WORLD, filename_write, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fout);
		if(err) {
			printf("Rank %d: ERROR opening file %s\n", rank, filename_write);
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
		size_t new_no_b=0;
		size_t *new_bpd = malloc(sizeof(size_t)*3);
		new_bpd[0] = 0;
		new_bpd[1] = 0;
		new_bpd[2] = 0;
		size_t *old_bpd = malloc(sizeof(size_t)*3);
		old_bpd[0] = bricks_per_dimension[0];
		old_bpd[1] = bricks_per_dimension[1];
		old_bpd[2] = bricks_per_dimension[2];
		for(size_t i=0; i<lod; i++) {
			new_no_b = NBricks(old_bpd, 2, new_bpd);
			old_bpd[0] = new_bpd[0];
			old_bpd[1] = new_bpd[1];
			old_bpd[2] = new_bpd[2];
		}
		/* Calculate next LOD */
		if(GetNewLOD(fin, fout, bricks_per_dimension, new_bpd, new_no_b,BRICKSIZE,GHOSTCELLDIM,lod,rank,size) != 0) {
			printf("ERROR: Rank %d @ GetNewLOD()\n",rank);
			MPI_Abort(MPI_COMM_WORLD,EXIT_FAILURE);
		}
		lod++;
		free(new_bpd);
		free(old_bpd);
		MPI_File_close(&fin);
		MPI_File_close(&fout);
		if(new_no_b<2)
			finished=true;
	}
	if(rank==0) {
		lod_time = MPI_Wtime();
		lod_time -= start_time;
		printf("Initialization: %1.3f || Bricking: %1.3f || LOD: %1.3f\n",init_time, brick_time, lod_time);
	}
	free(myoffsets);
	free(VOLUME);
	MPI_Finalize();
	return EXIT_SUCCESS;
}
