#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "misc.h"

/* edge2 right, bot, back */
size_t ReadFile(MPI_File f, size_t *VOLUME, uint8_t *data, int src[3], const size_t GBSIZE, int edge[6], size_t bno)
{
	size_t cur = 0;
	const size_t ORIG_Y = src[1];
	const size_t ORIG_RIGHT = edge[1];
	for(size_t z=0; z<GBSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			edge[1] = ORIG_RIGHT;
			/* y_edge top GHOSTCELLS */
			if(edge[2] > 0 && y < edge[2] && src[1]>0)
				src[1]--;
			/* y_edge bot NO GHOSTCELLS */
			if(edge[3] > 0 && src[1] >= VOLUME[1]) {
				src[1]--;
			}
			/* z_edge back NO GHOSTCELLS */
			if(src[2] >= VOLUME[2]) {
				src[2]--;
			}/* y_edge bot NO GHOSTCELLS */
			if(src[1] >= VOLUME[1]) {
				src[1]--;
			}
			size_t LINE = GBSIZE-edge[0]-edge[1];
			while(src[0]+LINE>VOLUME[0]) {
				LINE--;
				edge[1]++;
			}
			//printf("%d || %d %d %d || %d || %d \n",bno, src[0], src[1], src[2], LINE, edge[1]);
			const size_t COPY_FROM = src[0]+src[1]*VOLUME[0]+src[2]*VOLUME[0]*VOLUME[1];
			MPI_File_seek(f, COPY_FROM, MPI_SEEK_SET);
			/*TODO check fread*/
           		MPI_File_read(f, &data[edge[0]+cur], LINE, MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);

			/* x_edge left */
			if(edge[0]>0) {
				for(size_t i=0; i<edge[0]; i++)
					data[cur+i] = data[edge[0]+cur];
			}
			/* x_edge right */
			if(edge[1]>0) {
				for(size_t i=0; i<edge[1]; i++) {
					data[edge[0]+cur+LINE+i] = data[edge[0]+cur+LINE-1];
				}
			}
			cur+=GBSIZE;
			src[1]++;
			/* y_edge top NO GHOSTCELLS */
			if(edge[2] > 0 && y < edge[2] && src[1]==1)
					src[1]--;
		}
		src[1] = ORIG_Y;
		src[2]++;

		/* z_edge front NO GHOSTCELLS*/
		if(edge[4] > 0 && z < edge[4] && src[2]==1) {
			src[2]--;
		}
	}
	return 0;
}



size_t brick(MPI_File fin, MPI_File fout, size_t mystart, size_t mybricks, const size_t GBSIZE, const size_t GDIM, const size_t BSIZE, size_t *VOLUME, size_t *bpd)
{
	for(size_t no_b=mystart; no_b<mystart+mybricks; no_b++) {
		/* offset for output file */
		int output_file_offset = no_b*GBSIZE*GBSIZE*GBSIZE;
		MPI_File_seek(fout, output_file_offset, MPI_SEEK_SET);
		/* coordinates of current brick */
		size_t b[3];
		GetBrickCoordinates(b,bpd,no_b);
		int src[3] =
			{(b[0]%bpd[0]*BSIZE)-GDIM,
			 (b[1]%bpd[1]*BSIZE)-GDIM,
			 (b[2]%bpd[2]*BSIZE)-GDIM};

		int edge[6] = {0,0,0,0,0,0};
		CheckEdge(b, edge, GDIM, VOLUME, BSIZE, src, no_b);
		uint8_t *data = malloc(sizeof(uint8_t) * GBSIZE * GBSIZE * GBSIZE);
		/* read from input file */
		if(ReadFile(fin, VOLUME, data, src, GBSIZE, edge, no_b) != 0)
			return EXIT_FAILURE;
		/* write into output file */
		size_t offset = no_b*GBSIZE*GBSIZE*GBSIZE;
		WriteBrick(fout, data, GBSIZE*GBSIZE*GBSIZE, offset);
		free(data);
	}
}
