#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "misc.h"
#include "mpi_init.h"

size_t Get8Bricks(MPI_File fin, MPI_File fout, uint8_t *data, size_t b[3], size_t last_bpd[3], const size_t current_brick, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, size_t lod)
{
	size_t write_offset = 0;
	for(size_t brickZ=0; brickZ<2; brickZ++) {
		for(size_t brickY=0; brickY<2; brickY++) {
			for(size_t brickX=0; brickX<2; brickX++) {
				size_t current_brick[3] = {b[0]*2+brickX, b[1]*2+brickY, b[2]*2+brickZ};
				size_t current_id = GetBrickId(current_brick, last_bpd);
				size_t offset = current_id*GBSIZE*GBSIZE*GBSIZE;
				if(ReadLine(fin, data, write_offset, GBSIZE*GBSIZE*GBSIZE, offset) != 0) {
					printf("ReadLine error\n");
					return 1;
				}
				write_offset+=GBSIZE*GBSIZE*GBSIZE;
				//printf("%d %d\n", write_offset, offset);
			}
		}
	}
	return 0;
}

size_t GetGhostcells(MPI_File f, uint8_t *data, const size_t READ_DATA_FROM, uint8_t *ghostcells, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, size_t b[3], size_t id, size_t old_bpd[3], const size_t EDGE[6])
{
	size_t write_at = 0;
	/* front */
	if(EDGE[4]==1&&GDIM>0) {
		if(b[2]>0) {
			size_t new_b[3] = {b[0], b[1], b[2]-1};
			size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-2*GDIM*GBSIZE*GBSIZE;
			size_t line = 2*GDIM*GBSIZE*GBSIZE;
			if(ReadLine(f, ghostcells, write_at, line, offset) != 0)
			printf("error front gc\n");
			write_at+=line;
		} else {
			size_t offset = READ_DATA_FROM+GDIM*GBSIZE*GBSIZE;
			size_t line = GDIM*GBSIZE*GBSIZE;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
		}
	}
	/* back */
	if(EDGE[5]==1 && GDIM>0) {
		if(b[2]+1 < old_bpd[2]) {
			size_t new_b[3] = {b[0], b[1], b[2]+1};
			size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE;
			size_t line = 2*GDIM*GBSIZE*GBSIZE;
			if(ReadLine(f, ghostcells, write_at, line, offset) != 0)
			printf("error back gc\n");
			write_at+=line;
		} else {
			size_t offset = READ_DATA_FROM+GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE;
			size_t line = GDIM*GBSIZE*GBSIZE;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
		}
	}

	for(size_t z=0; z<BSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/* top */
			if(EDGE[2] == 1 && y < GDIM) {
				if(b[1] > 0) {
					size_t new_b[3] = {b[0], b[1]-1, b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE + z*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-2*GDIM*GBSIZE;
					size_t line = 2*GDIM*GBSIZE;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error top gc\n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+GDIM*GBSIZE;
					size_t line = GDIM*GBSIZE;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
			/* bottom */
			if(EDGE[3] == 1 && y>=(BSIZE+GDIM)) {
				if(b[1]+1 < old_bpd[1]) {
					size_t new_b[3] = {b[0], b[1]+1, b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+GDIM*GBSIZE;
					size_t line = 2*GDIM*GBSIZE;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error bot gc\n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-GBSIZE;
					size_t line = GBSIZE;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
		}
	}
	for(size_t z=0; z<BSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/* left */
			if(EDGE[0] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
				if(b[0] > 0) {
					size_t new_b[3] = {b[0]-1, b[1], b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE+z*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
					size_t line = 2*GDIM;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error left gc\n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GDIM;
					size_t line=GDIM;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
			/* right */
			if(EDGE[1] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
				if(b[0]+1 < old_bpd[0]) {
					size_t ob[3] = {b[0]+1, b[1], b[2]};
					size_t offset = GetBrickId(ob, old_bpd)*GBSIZE*GBSIZE*GBSIZE+(GDIM+z)*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
					size_t line = 2*GDIM;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error right gc \n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM;
					size_t line = GDIM;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
		}
	}
	return 0;
}

void ReorganiseArray(uint8_t *data, uint8_t *ghostcells, uint8_t *finalData, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, const size_t EDGE[6], size_t b[3])
{
	const size_t ID = b[0]+b[1]*2+b[2]*4;
	size_t read_data_at = GDIM*GBSIZE*GBSIZE+GDIM*GBSIZE+GDIM;
	size_t helper = ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4;
	size_t front[4] = {
		0,
		GBSIZE,
		2*GBSIZE*GBSIZE,
		2*GBSIZE*GBSIZE+GBSIZE};
	size_t back[4] = {
		2*helper-8*GDIM*GBSIZE*GBSIZE, 
		2*helper-8*GDIM*GBSIZE*GBSIZE+GBSIZE, 
		2*helper-8*GDIM*GBSIZE*GBSIZE+2*GBSIZE*GBSIZE, 
		2*helper-8*GDIM*GBSIZE*GBSIZE+2*GBSIZE*GBSIZE+GBSIZE};
	size_t top[8] = {
		4*2*GDIM*GBSIZE*GBSIZE,
		4*2*GDIM*GBSIZE*GBSIZE+GBSIZE,
		4*2*GDIM*GBSIZE*GBSIZE+4*GDIM*GBSIZE+4*GBSIZE*BSIZE,
		4*2*GDIM*GBSIZE*GBSIZE+4*GDIM*GBSIZE+4*BSIZE*GBSIZE+GBSIZE,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+GBSIZE,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+ 4*GDIM*GBSIZE+4*GBSIZE*BSIZE,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+ 4*GDIM*GBSIZE+4*GBSIZE*BSIZE+GBSIZE};
	size_t left[8] = {
		top[0]+2*(GBSIZE*2*GDIM),
		top[0]+2*(GBSIZE*2*GDIM)+2*GDIM+2*BSIZE,
		top[0]+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE),
		top[0]+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE)+2*GDIM+2*BSIZE,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM),
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+2*GDIM+2*BSIZE,
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE),
		((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE)+2*GDIM+2*BSIZE
	};
	size_t data_start[8] = {
		left[0]+2*GDIM,
		left[0]+2*GDIM+BSIZE,
		left[2]+2*GDIM,
		left[2]+2*GDIM+BSIZE,
		left[4]+2*GDIM,
		left[4]+2*GDIM+BSIZE,
		left[6]+2*GDIM,
		left[6]+2*GDIM+BSIZE};
	size_t ct = 0;
	for(size_t z=0 ;z<2*GDIM; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/*FRONT*/
			if(EDGE[4]==1) {
				memcpy(&finalData[front[ID]+z*4*GBSIZE*GBSIZE+y*2*GBSIZE], &ghostcells[z*GBSIZE*GBSIZE+y*GBSIZE], GBSIZE);
				ct+=GBSIZE;
			}
			/*BACK*/
			if(EDGE[5]==1) {
				memcpy(&finalData[back[ID-4]+z*4*GBSIZE*GBSIZE+y*2*GBSIZE], &ghostcells[z*GBSIZE*GBSIZE+y*GBSIZE], GBSIZE);
				ct+=GBSIZE;
			}
		}
	}
	for(size_t z=0; z<BSIZE; z++) {
		for(size_t y=0; y<GBSIZE+2*GDIM; y++) {
			/* TOP && BOTTOM */
			if((EDGE[2]==1 || EDGE[3]==1) && y<2*GDIM) {
			memcpy(&finalData[top[ID]+z*4*GBSIZE*GBSIZE+y*2*GDIM*GBSIZE], &ghostcells[2*GDIM*GBSIZE*GBSIZE+z*2*GDIM*GBSIZE+y*GDIM*GBSIZE], GBSIZE);
			ct+=GBSIZE;
			}
			/* LEFT && RIGHT */
			if((EDGE[0]==1 || EDGE[1]==1) && y>=2*GDIM && y<2*GDIM+BSIZE) {
			memcpy(&finalData[left[ID]+z*((2*2*GDIM+2*BSIZE)*2*BSIZE+2*(2*2*GDIM*GBSIZE))+(y-2*GDIM)*2*GDIM*GBSIZE], &ghostcells[2*GDIM*GBSIZE*GBSIZE+2*GDIM*GBSIZE*BSIZE+z*2*GDIM*2*GDIM+(y-2*GDIM)*2*GDIM], 2*GDIM);
			ct+=2*GDIM;
			}
			/* DATA */
			if(y>=2*GDIM && y<2*GDIM+BSIZE) {
			memcpy(&finalData[data_start[ID]+z*((2*2*GDIM+2*BSIZE)*2*BSIZE+2*(2*2*GDIM*GBSIZE))+(y-2*GDIM)*2*GDIM*GBSIZE], &data[ID*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+(y+GDIM-2*GDIM)*GBSIZE+GDIM], BSIZE);
			ct+=BSIZE;
			}
		}
	}
}

uint8_t CollapseVoxels(uint8_t *data, size_t off, size_t b[3], const size_t GBSIZE)
{
	size_t out = 0;
	size_t o = b[0]+b[1]*GBSIZE*2+b[2]*GBSIZE*2*GBSIZE*2;
	for(size_t z=0; z<2; z++) {
		for(size_t y=0; y<2; y++) {
			for(size_t x=0; x<2; x++) {
				out+=data[o+x+y*GBSIZE+z*GBSIZE*GBSIZE];
			}
		}
	}
	return out/8;
}

size_t LowerLOD(MPI_File f, uint8_t *data, const size_t GBSIZE, size_t output_offset)
{
	for(size_t z=0; z<GBSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			for(size_t x=0; x<GBSIZE; x++) {
				size_t small_b[3] = {x*2,y*2,z*2};
				uint8_t out[1] = {CollapseVoxels(data, 0, small_b, GBSIZE)};
				if(WriteBrick(f, out, 1, output_offset) != 0)
					return 1;
				output_offset++;
			}
		}
	}
	return 0;
}

size_t GetNewLOD(MPI_File fin, MPI_File fout, size_t *old_bpd, size_t *current_bpd, size_t current_no_bricks, const size_t BSIZE, const size_t GDIM, size_t lod, int rank, int size)
{
	const size_t GBSIZE=BSIZE+2*GDIM;
	size_t last_bpd[3] = {old_bpd[0], old_bpd[1], old_bpd[2]};
	for(size_t i=0; i<lod-1;i++) {
		NBricks(last_bpd, 2, last_bpd);
	}
	size_t mybricks = 42;
	size_t mystart;
	size_t *starting_brick;
	size_t *bricks;
	if(rank==0) {
		starting_brick = malloc(sizeof(size_t)*size);
		bricks = malloc(sizeof(size_t)*size);
		getBricks(bricks, starting_brick, size, current_no_bricks);
		mybricks=bricks[0];
		mystart=starting_brick[0];
		if(size>1) {
			for(size_t i=1; i<size; i++) {
				printf("====%d %d\n",starting_brick[i], bricks[i]);
				if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending # of blocks\n");
					return 1;
				}
				if(MPI_Send(&starting_brick[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending starting brick\n");
					return 1;
				}
			}
		}
		free(starting_brick);
		free(bricks);
	} else {
		int count;
		MPI_Status stat;
		printf("%d %d %d\n",rank, mybricks, mystart);
		MPI_Recv(&mybricks, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
			fprintf(stderr, "Rank %d: ERROR receiving # of bricks\n", rank);
			return 1;
		}
		MPI_Recv(&mystart, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
			fprintf(stderr, "Rank %d: ERROR receiving starting brick\n", rank);
			return 1;
		}
		printf("%d %d %d\n",rank, mybricks, mystart);
	}
	printf("Rank %d / Mystart %d / Mybricks %d\n", rank, mystart, mybricks);
	/* for each new brick */
	for(size_t no_b=mystart; no_b < mystart+mybricks; no_b++) {
		size_t b[3];
		GetBrickCoordinates(b, current_bpd, no_b);
		uint8_t *data = malloc(sizeof(uint8_t)*GBSIZE*GBSIZE*GBSIZE*8);
		if(Get8Bricks(fin, fout, data, b, last_bpd, no_b, BSIZE, GDIM, GBSIZE, lod) != 0) {
			printf("Error@Get8Bricks()\n");
			return 1;
		}
		uint8_t *finalData = malloc(sizeof(uint8_t) * ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*8);
		for(size_t brickZ=0; brickZ<2; brickZ++) {
			for(size_t brickY=0; brickY<2; brickY++) {
				for(size_t brickX=0; brickX<2; brickX++) {
					const size_t EDGE[6] = {(brickX+1)%2, brickX%2, (brickY+1)%2, brickY%2, (brickZ+1)%2, brickZ%2};
					const size_t READ_DATA_FROM = (brickX+brickY*2+brickZ*4)*GBSIZE*GBSIZE*GBSIZE;
					//printf("%d %d %d :: %d\n", brickX, brickY, brickZ, READ_DATA_FROM);
					const size_t FINAL_DATA_START_AT = (brickX+brickY*2+brickZ*4)*(GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE;
					uint8_t *ghostcells = malloc(sizeof(uint8_t)*(2*GDIM*GBSIZE*GBSIZE + BSIZE*GDIM*2*GDIM*GBSIZE+BSIZE*BSIZE*2*GDIM));
					size_t current_brick[3] = {brickX+b[0]*2,brickY+b[1]*2,brickZ+b[2]*2};
					size_t current_id = GetBrickId(current_brick, old_bpd);
					if(GetGhostcells(fin, data, READ_DATA_FROM, ghostcells, BSIZE, GDIM, GBSIZE, current_brick, current_id, last_bpd, EDGE) != 0) {
					printf("GetGhostcells error\n");
					return 1;
					}
					size_t b[3] = {brickX, brickY, brickZ};
					ReorganiseArray(data, ghostcells, finalData, BSIZE, GDIM, GBSIZE, EDGE, b );
					free(ghostcells);
				}
			}
		}
	LowerLOD(fout, finalData, GBSIZE, no_b*GBSIZE*GBSIZE*GBSIZE);
	free(data);
	free(finalData);
	}
	return 0;
}
