#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include "io.h"
/*
every voxel 1byte
engine 256x256x128
nucleon 41x41x41
fuel 64x64x64
*/


void printArr(size_t *arr, size_t size)
{
	for(size_t i=0; i<size; i++) {
		for(size_t j=0; j<size; j++) {
			for(size_t k=0; k<size; k++) {
				size_t idx = i*size*size+j*size+k;
				if(arr[idx] < 10) {
				printf("0%d ",arr[idx]);
				} else {
				printf("%d ", arr[idx]);
				}
			}
		printf("\n");
		}
	printf("\n");
	}
}

void printArr2(size_t *arr, size_t size_c) {
	size_t ct = 0;
	size_t ctt = 0;
	size_t cttt = 0;
	for (size_t i=0; i<size_c; i++) {
		size_t idx = i;
		if(arr[idx] < 10) {
		printf("0%d ",arr[idx]);
		} else {
		printf("%d ", arr[idx]);
		}
		ct++;
		if(ct==4) {
			printf("\n");
			ct=0;
			ctt++;
		}		
		if(ctt==4) {
			printf("\n");
			ctt=0;
			cttt++;
		}
		if(cttt==4) {
			printf("-------------+\n");
			cttt=0;
		}
	}
}


/* voxel size
* 1 for now */
const size_t getBytes(){
	return 1;
}

/* how many bricks are needed? */
size_t nbricks(const size_t vol[3], size_t bsize[3], size_t nb[3])
{
	if(vol[0] < bsize[0] || vol[1] < bsize[1] || vol[2] < bsize[2]) {
		return -1;
	}
	for(size_t i=0; i<3; i++) {
		nb[i] = vol[i] / bsize[i];
		if(vol[i] % bsize[i] != 0) {
			nb[i]++;
		}
	}
	return 0;
}

/* subtract # of ghost cells from bricksize */
size_t nbricksgc(size_t bs[3], const size_t gc)
{
	for(int i=0; i<3; i++) {
		bs[i]-=gc;
		if(bs[i]<1) return -1;
	}
	return 0;
}

void edge_padding(uint8_t *dest, size_t to, size_t n)
{
	//printf("Edge padding\n");
	for(int i=0; i<n; i++) {
		dest[to] = 0;
		//printf("%d ", dest[to]);
		to++;
	}
		//printf("\n");
}

/* edge[1] left; edge[0] right */
void copyline(uint8_t *dest, uint8_t *src, size_t to, size_t from, size_t n, int edge[3]) {
		n-=edge[1];
		for(int i=edge[1]; i>0; i--) {
			dest[to] = 0;
			//printf("%d ", from);
			to++;
		}
		if(edge[1] == 0) {
			from--;
		}
		for(int i=0; i<n-edge[0]; i++) {
			dest[to] = src[from];
			//printf("%d ", from);
			to++;
			from++;
		}
		for(int i=n-edge[0]; i<n; i++) {
			dest[to] = 0;
			//printf("%d ", from);
			to++;
		}
		//printf("\n");
}

void copyline_pad(uint8_t *dest, uint8_t *src, size_t to, size_t n) {
		//printf("Copyline_pad\n");
		for(size_t i=0; i<n; i++) {
			dest[to] = 0;
			//printf("%d ", dest[to]);
			to++;
		}
		//printf("\n");
}

int main(int argc, char* argv[])
{
	/* init
	* ./bricked vol_size^3 brick_dimension^3 ghostcelldim filename */
	if(argc!=5) {
		printf("Check input\n");
		return EXIT_FAILURE;
	}
	char *endptr = NULL;
	const size_t size = strtol(argv[1], &endptr, 10);
	const size_t brickdim = strtol(argv[2], &endptr, 10);
	const size_t ghostcelldim = strtol(argv[3], &endptr, 10);
	const size_t size_c = size*size*size;
	const size_t volume[3] = {size, size, size};

	/*hard coded init*/
	uint8_t *arr;
	arr = malloc(sizeof(uint8_t)*size_c*1);
	if(read_data(argv[4], size, arr) != 0) return EXIT_FAILURE;
	/* */

	size_t bsize[3] = {brickdim, brickdim, brickdim};
	size_t nb[3];
	/* subtract ghost cells for number of bricks calculation */
	/* x2 because we need both sides
	* (left&&right, top&&bot, front&&back) */
	if(nbricksgc(bsize, ghostcelldim*2) != 0) {
		printf("ERROR determining brick size with ghost cells!\nCheck your input\n");
		return EXIT_FAILURE;
	}
	if(nbricks(volume, bsize, nb) != 0) {
		printf("ERROR determining brick size!\nCheck your input\n");
		return EXIT_FAILURE;
	}
	const size_t no_bricks = nb[0]*nb[1]*nb[2];
	printf("\nNumber of bricks:%d\nDimension without GC:%dx%dx%d\n", no_bricks, bsize[0], bsize[1], bsize[2]);

	/* offset for parallel
	* hardcoded here */
	size_t offset[3] = {0,0,0};
	size_t goffset[3] = {offset[0]-ghostcelldim, offset[1]-ghostcelldim, offset[2]-ghostcelldim};

	/* original offset */
	size_t orig_offset[3] = {offset[0], offset[1], offset[2]};

	/* get voxel bytes */
	const size_t vsize = getBytes();

	/* main loop */
	uint8_t *bricked;
	size_t gbsize[3] = {bsize[0]+2*ghostcelldim, bsize[1]+2*ghostcelldim, bsize[2]+2*ghostcelldim};
	/* scanline */
	const size_t line = vsize * gbsize[0];
	size_t bricked_size = gbsize[0]*gbsize[1]*gbsize[2]*no_bricks;
	size_t counter = 0;
	bricked = malloc(sizeof(uint8_t)*bricked_size);

	for(size_t no_b = 0; no_b<no_bricks; no_b++) {
		offset[0] = orig_offset[0];
		offset[1] = orig_offset[1];
		offset[2] = orig_offset[2];
		//printf("%d || %d %d %d\n", no_b,offset[0],offset[1],offset[2]);
		/* x_edge left, y_edge top, z_edge front */
		int start[3] = {offset[0]-ghostcelldim,offset[1]-ghostcelldim,offset[2]-ghostcelldim};
		const int orig_start_y = start[1];
		int edge[3] = {0,0,0};
		/* x_edge right */
		if(offset[0]+gbsize[0]-ghostcelldim>=size) {
			edge[0] = offset[0]+gbsize[0]-ghostcelldim-size;
		}
		if(start[0] < 0) {
			edge[1] = start[0] * -1;
		}
		for(size_t z=0; z<gbsize[2]; z++) {
			/* z pad */
			if(start[2] < 0) {
				edge_padding(bricked, (z*gbsize[1]*gbsize[0])*vsize+no_b*gbsize[0]*gbsize[1]*gbsize[2], gbsize[0]*gbsize[1]);
				start[2]++;
			/* z pad */
			} else  if (offset[2]>=size) {
				edge_padding(bricked, (z*gbsize[1]*gbsize[0])*vsize+no_b*gbsize[0]*gbsize[1]*gbsize[2], gbsize[0]*gbsize[1]);
			} else {
				/* ghostcell page */
				if(offset[2]-(ghostcelldim-z)>=0 && z<ghostcelldim) {
					offset[2]=offset[2]-ghostcelldim;	
				}
				for(size_t y=0; y<gbsize[1]; y++) {
					const size_t copy_to = (z*gbsize[1]*gbsize[0]+y*gbsize[0])*vsize+no_b*gbsize[0]*gbsize[1]*gbsize[2];
					/* y_edge! top */
					if(start[1] < 0) {
						copyline_pad(bricked, arr, copy_to, line);
						start[1]++;
					/* y_edge! bot */
					} else if(offset[1]>=size) {
						copyline_pad(bricked, arr, copy_to, line);
					/*ghostcell top*/
					} else if(y<ghostcelldim) {
						const size_t copy_from = (offset[0] + (offset[1]-(ghostcelldim-y))*size + offset[2]*size*size) *vsize;
						copyline(bricked, arr, copy_to, copy_from, line, edge);;
					} else {
						const size_t copy_from = (offset[0] + offset[1]*size + offset[2]*size*size) *vsize;
						copyline(bricked, arr, copy_to, copy_from, line, edge);
						offset[1]++;
					}
				}
           		/* reset y
			* increment z */
			start[1] = orig_start_y;
			offset[1] = orig_offset[1];
			offset[2]++; 
		    }
		}
		orig_offset[0] += bsize[0];
		if(orig_offset[0]>=size) {
			orig_offset[1] += bsize[1];
			if(orig_offset[1] >= size) {
				orig_offset[2] += bsize[2];
				orig_offset[1] = 0;
			}
		orig_offset[0] = 0;
		}
	}
	//printArr(arr, size);
	//printArr2(bricked, bricked_size);
	//printf("\nCounted:%d\n", counter*gbsize[0]);
	if(write_data(gbsize[0], no_bricks, bricked) != 0) {
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

