#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>

/* how many bricks are needed? */
size_t NBricks(size_t *VOLUME, const size_t BRICKSIZE, size_t *bdim)
{
	if(VOLUME[0] < BRICKSIZE || VOLUME[1] < BRICKSIZE || VOLUME[2] < BRICKSIZE)
		return 1;
	for(size_t i=0; i<3; i++) {
		bdim[i] = VOLUME[i] / BRICKSIZE;
		if(VOLUME[i] % BRICKSIZE != 0) {
		bdim[i]++;
		}
	}
	//printf("No Bricks : %d\n", bdim[0]*bdim[1]*bdim[2]);
	//printf("X %d Y %d Z %d\n", bdim[0],bdim[1],bdim[2]);
	return bdim[0]*bdim[1]*bdim[2];
}

/* right, bot, back */
void CheckEdge(size_t b[3], int edge[6], const size_t GDIM, size_t *VOLUME, const size_t BRICKSIZE, int src[3]) {
	/* left top front */
	for(size_t i=0; i<3; i++) {
		if(src[i]<0) {
			edge[i*2] = (src[i])*-1;
			src[i]=0;

		}
	}
	/* right bot back */
	for(int i=0; i<3; i++) {
		if(b[i]+GDIM>VOLUME[i]/BRICKSIZE)
			edge[i*2+1]=(int)GDIM;
	}
}

void GetBrickCoordinates(size_t b[3], size_t bpd[3], size_t id)
{
	b[0] = id%bpd[0];
	b[1] = (id/bpd[0])%bpd[1];
	b[2] = (id/(bpd[0]*bpd[1]))%bpd[2];
}

size_t GetBrickId(size_t b[3], size_t bpd[3])
{
	return b[0]+b[1]*bpd[0]+b[2]*bpd[0]*bpd[1];
}


size_t WriteBrick(MPI_File f, uint8_t *data, size_t length, size_t off)
{
	MPI_File_seek(f, off, MPI_SEEK_SET);
	MPI_Status stat;
	int count;
	MPI_File_write(f, &data[0], (int)length, MPI_UNSIGNED_CHAR, &stat);
	MPI_Get_count(&stat, MPI_UNSIGNED_CHAR, &count);
	if(count!=(int)length) {
		fprintf(stderr, "ERROR @ writing brick to file\n");
		return 1;
	}

	return 0;
}

size_t ReadLine(MPI_File f, uint8_t *data, size_t cur, size_t LINE, size_t off)
{
	MPI_File_seek(f, off, MPI_SEEK_SET);
	MPI_Status stat;
	int count;
	MPI_File_read(f, &data[cur], (int)LINE, MPI_UNSIGNED_CHAR, &stat);
	MPI_Get_count(&stat, MPI_UNSIGNED_CHAR, &count);
	if(count!=(int)LINE) {
		fprintf(stderr, "ERROR @ reading file\n");
		return 1;
	}
	return 0;
}
