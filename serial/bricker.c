#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "file_io.h"
#include "rw.h"
#include "multires.h"

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

void GetBrickCoordinates(size_t b[3], size_t bpd[3], size_t id)
{
	b[0] = id%bpd[0];
	b[1] = (id/bpd[0])%bpd[1];
	b[2] = (id/(bpd[0]*bpd[1]))%bpd[2];
}

size_t GetBrickId(size_t b[3], const size_t bpd[3])
{
	return b[0]+b[1]*bpd[0]+b[2]*bpd[0]*bpd[1];
}

/* right, bot, back */
void CheckEdge2(size_t b[3], size_t edge[3], const size_t GDIM, const size_t VOLUME[3], const size_t BRICKSIZE) {
	for(size_t i=0; i<3; i++) {
		size_t n = VOLUME[i]/BRICKSIZE;
		size_t cmp = 0;
		if(VOLUME[i]%BRICKSIZE==0) cmp++;
		if(b[i] + cmp == n) {
			edge[i] = BRICKSIZE+GDIM-(VOLUME[i]%BRICKSIZE);
		} 
	} 
}

void ResetOffset(size_t o[3], size_t oo[3]) {
	for(size_t i=0; i<3; i++)
		o[i] = oo[i];
}

/* edge2 right, bot, back */
size_t ReadBrick(FILE *f, const size_t VOLUME[3], uint8_t *data, int src[3], const size_t GBSIZE, size_t e[3], size_t bno)
{
	size_t cur = 0;
	/* left, top, front */
	int e2[3] = {0,0,0};
	for(size_t i=0; i<3; i++) {
		if(src[i] < 0) {
			e2[i] = src[i]*-1;
			src[i] = 0;
		}
	}
	/* left, right, top, bot, front, back */
	size_t edge[6] = {e2[0], e[0], e2[1], e[1], e2[2], e[2]};
	/* top, bot */
	const size_t ORIG_Y = src[1];
	for(size_t z=0; z<GBSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/* y_edge top GHOSTCELLS */
			if(edge[2] > 0 && y < edge[2] && src[1]>0) 
					src[1]--;
			/* y_edge bot NO GHOSTCELLS */
			if(edge[3] > 0 && src[1] >= VOLUME[1]) {
					src[1]--;
			}	
			/* z_edge back NO GHOSTCELLS */
			if(edge[5]>0 &&  src[2] >= VOLUME[2]) {
				src[2]--;
			}
			const size_t LINE = GBSIZE-edge[0]-edge[1];
 		
			const size_t COPY_FROM = src[0]+src[1]*VOLUME[0]+src[2]*VOLUME[0]*VOLUME[1];
			int a;
			fseek(f, COPY_FROM, SEEK_SET);
			if(a = fread(&data[edge[0]+cur], sizeof(uint8_t), LINE, f) != LINE) {
				printf("ERROR @ %d: Read %d of %d\nPosition %d %d %d\nLoop Y: %d Z: %d\nLEFT %d RIGHT %d\n%d", bno,a, LINE, src[0], src[1],src[2],y,z,edge[0],edge[1],COPY_FROM);		
				return 1;
			}
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

size_t WriteBrick(FILE *f, uint8_t *data, size_t length, size_t buf)
{
	fseek(f, buf, SEEK_SET);
	if(fwrite(&data[0], sizeof(uint8_t), length, f) != length)
		printf("fwrite error\n");
	return 0;
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

	//WriteMetadata(VOLUME,BRICKDIM,GHOSTCELLDIM,numberofbricks,bricks_per_dimensionension[3]);
	
	size_t offset[3] = {0,0,0};
	size_t orig_offset[3] = {0,0,0};

	size_t GBSIZE=BRICKSIZE+2*GHOSTCELLDIM;
	printf("Bricksize %d || GBricksize %d\n", BRICKSIZE, GBSIZE);
	
	FILE *fpi = NULL;
	fpi = fopen(argv[6],"rb");
	if(fpi == NULL) {
		printf("Couldn't open file %s\n", argv[6]);
		return EXIT_FAILURE;	
	}

	FILE *fpo = NULL;
	char fn[256];
	sprintf(fn, "%d.raw", 1);
	fpo = fopen(fn,"rwb");
	if(fpo == NULL) {
		printf("Couldn't open file %s.raw\n", fn);
		return EXIT_FAILURE;
	}
	
	
	
	for(size_t no_b=0; no_b<numberofbricks; no_b++) {
		size_t b[3];
		GetBrickCoordinates(b,bricks_per_dimension,no_b);
		size_t buffer = no_b*GBSIZE*GBSIZE*GBSIZE;
		size_t edge[3] = {0,0,0};
		CheckEdge2(b, edge, GHOSTCELLDIM, VOLUME, BRICKSIZE);
		int src[3] = 
			{(b[0]%bricks_per_dimension[0]*BRICKSIZE)-GHOSTCELLDIM,
			 (b[1]%bricks_per_dimension[1]*BRICKSIZE)-GHOSTCELLDIM,
			 (b[2]%bricks_per_dimension[2]*BRICKSIZE)-GHOSTCELLDIM};
		uint8_t *data = malloc(sizeof(uint8_t) * GBSIZE * GBSIZE * GBSIZE);		
		if(ReadBrick(fpi, VOLUME, data, src, GBSIZE, edge, no_b) != 0) 
			return EXIT_FAILURE;
		WriteBrick(fpo, data, GBSIZE*GBSIZE*GBSIZE, buffer);
		free(data);
		//printf("Brick %d done\n", no_b);
	}
	fclose(fpi);

	FILE *fpm = NULL
	fpm = fopen("multi.raw", "rwb");
	if(fpm == NULL) {
		printf("Couldn't open file multi.raw\n");
		return EXIT_FAILURE;
	}

	CalcMultiRes();
	
		
	
	fclose(fpo);
	fclose(fpm);	
}
