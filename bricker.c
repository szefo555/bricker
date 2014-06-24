#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strings.h>
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
	for (size_t i=0; i<size_c; i++) {
				size_t idx = i;
				if(arr[idx] < 10) {
				printf("0%d ",arr[idx]);
				} else {
				printf("%d ", arr[idx]);
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

void edge_padding(size_t *dest, size_t to, size_t n)
{
	for(int i=0; i<n; i++) {
		dest[to] = 0;
		to++;
	}
}

/* check for y_edge first (edge[1]) 
* if false copy full line - x_edge and pad rest of x_edge
* if true pad whole line */
void copyline(size_t *dest, size_t *src, size_t to, size_t from, size_t n, size_t edge[3]) {
	if(edge[1] == 0) {
		for(int i=0; i<n-edge[0]; i++) {
			dest[to] = src[from];
			to++;
			from++;
		}
		for(int i=n-edge[0]; i<n; i++) {
			dest[to] = 0;
			to++;
		}
	} else {
		/* pad line */
		for(int i=0; i<n; i++) {
			dest[to] = 0;
			to++;
		}
	}
}

int main(int argc, char* argv[]) 
{
	/* init 
	* ./bricked vol_size^3 brick_dimension^3 */	
	if(argc!=3) {
		printf("Check input\n");
		return EXIT_FAILURE;
	}
	char *endptr = NULL;
	const size_t size = strtol(argv[1], &endptr, 10);
	const size_t brickdim = strtol(argv[2], &endptr, 10);
	const size_t size_c = size*size*size;
	const size_t volume[3] = {size, size, size};
	
	/*hard coded init*/
	size_t *arr;
	arr = malloc(sizeof(size_t)*size_c*1);
	for(int i=0; i<size_c; i++) {
		arr[i] = i+1;
	}
	/* x2 because we need both sides 
	* (left&&right, top&&bot, front&&back) */	
	const size_t ghostcelldim = 2 * 1;
	/* */	

	size_t bsize[3] = {brickdim, brickdim, brickdim};
	size_t nb[3];
	/* subtract ghost cells for number of bricks calculation */
	if(nbricksgc(bsize, ghostcelldim) != 0) {
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
	
	/* original offset */
	size_t orig_offset[3] = {offset[0], offset[1], offset[2]};

	/* get voxel bytes */
	const size_t vsize = getBytes();

	/* scanline */
	const size_t line = vsize * bsize[0];

	/* main loop */
	size_t *bricked;
	size_t bricked_size = (bsize[0]+ghostcelldim)*(bsize[1]+ghostcelldim)*(bsize[2]+ghostcelldim)*no_bricks;
	size_t counter = 0;
	bricked = malloc(sizeof(size_t)*bricked_size);
	for (size_t no_b=0; no_b<no_bricks; no_b++) {
		offset[0] = orig_offset[0];
		offset[1] = orig_offset[1];
		offset[2] = orig_offset[2];
		size_t edge[3] = {0,0,0};
		if(offset[0]+bsize[0]>=size) {
			/* x_edge! */ 
			edge[0] = offset[0]+bsize[0] - size;
		}
		for (size_t z=0; z<bsize[2]; z++) {
			/* reset y_edge */
			edge[1] = 0;
			/* z_edge */ 
			if(offset[2]>=size) {
				edge[2] = 1;
			}
			for(size_t y=0; y<bsize[1]; y++) {
				counter++;
				const size_t copy_to = (z*bsize[1]*bsize[0] + y*bsize[0]) *vsize + no_b*bsize[0]*bsize[1]*bsize[2];
				if(edge[2] == 0) {
					const size_t copy_from = (offset[0] + offset[1]*size + offset[2]*size*size) *vsize;
					if(offset[1]>=size) {
						/* y_edge! */
						edge[1] = 1;
					}
					copyline(bricked, arr, copy_to, copy_from, line, edge);
					/* increment y */
					offset[1]++;
				} else {
					/* pad a full page (x*y) */
					edge_padding(bricked, copy_to, bsize[0]*bsize[1]);
				}
			}
		/* reset y
		* increment z */
		offset[1] = orig_offset[1];
		offset[2]++;	 
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
	printArr(arr, size);
	printArr2(bricked, bricked_size);
	printf("\nCounted:%d\n", counter*bsize[0]);
	return EXIT_SUCCESS;
}
