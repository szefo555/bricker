#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

void getOffsets(size_t *o, size_t *b, const size_t *VOLUME, const size_t BS, size_t size)
{
    o[0] = 0;
    o[1] = 0;
    o[2] = 0;
    for(size_t i=1; i<size; i++) {
        o[i*3+0]=o[(i-1)*3+0];
        o[i*3+1]=o[(i-1)*3+1];
        o[i*3+2]=o[(i-1)*3+2];
        size_t no_b = b[i-1];
        while(no_b > 0) {
            o[i*3+0] += BS;
            if(o[i*3+0]>=VOLUME[0]) {
                o[i*3+1] += BS;
                if(o[i*3+1] >= VOLUME[1]) {
                    o[i*3+2] += BS;
                    o[i*3+1] = 0;
                }
                o[i*3+0] = 0;
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

size_t Init_MPI(int rank, int size, size_t mybricks, size_t mystart, size_t *myoffsets, size_t *bricks, size_t *starting_brick, size_t *offsets, size_t numberofbricks, size_t *VOLUME, const size_t BRICKSIZE)
{
	if(rank==0) {
		getBricks(bricks, starting_brick, size, numberofbricks);
		mybricks=bricks[0];
		mystart=starting_brick[0];
		offsets = malloc(sizeof(size_t) * 3 * size);
		getOffsets(offsets, bricks, VOLUME, BRICKSIZE, size);
		myoffsets[0] = 0;
		myoffsets[1] = 0;
		myoffsets[2] = 0;
		if(size > 1) {
			for(size_t i=1; i<size; i++) {
				if(MPI_Send(&offsets[i*3+0], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending offset\n");
					return 1;
				}
				if(MPI_Send(&offsets[i*3+1], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending offset\n");
					return 1;
				}
				if(MPI_Send(&offsets[i*3+2], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending offset\n");
					return 1;
				}
				if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending # of blocks\n");
					return 1;
				}
				if(MPI_Send(&starting_brick[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
					fprintf(stderr, "ERROR sending starting brick\n");
					return 1;
				}
			}		
			free(offsets);
		} else {
			MPI_Status stat;
			int count;
			MPI_Recv(&myoffsets[0], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
			MPI_Recv(&myoffsets[1], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
			MPI_Recv(&myoffsets[2], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
			MPI_Get_count(&stat, MPI_UNSIGNED, &count);
			if(count!=1) {
				fprintf(stderr, "Rank %d: ERROR receiving offset\n", rank);
				return 1;
			}
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
		}			
	}
	return 0;
}
