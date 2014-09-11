#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "rw.h"
#include "multires.h"



/* right, bot, back */
void CheckEdge(size_t b[3], int edge[6], const size_t GDIM, const size_t VOLUME[3], const size_t BRICKSIZE, int src[3], size_t bno) {
	/* left top front */
	for(size_t i=0; i<3; i++) {
		if(src[i] < 0) {
			edge[i*2] = src[i]*-1;
			src[i] = 0;
		}
	}
	/* right bot back */
	for(size_t i=0; i<3; i++) {
		size_t n = VOLUME[i]/BRICKSIZE;
		/* NEEDS FIX ASAP */
		if(b[i]+GDIM >= n) {
			edge[i*2+1] = VOLUME[i]%BRICKSIZE+GDIM;
		} 
	} 
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



void ResetOffset(size_t o[3], size_t oo[3]) {
	for(size_t i=0; i<3; i++)
		o[i] = oo[i];
}

/* edge2 right, bot, back */
size_t ReadFile(FILE *f, const size_t VOLUME[3], uint8_t *data, int src[3], const size_t GBSIZE, int edge[6], size_t bno)
{
	printf("%d || %d %d %d || %d %d %d %d %d %d \n",bno, src[0], src[1], src[2], edge[0],edge[1],edge[2],edge[3],edge[4],edge[5]);
	size_t cur = 0;
	const size_t ORIG_Y = src[1];
	for(size_t z=0; z<GBSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			if(src[1] >= VOLUME[1])
				src[1]--;
			if(src[2] >= VOLUME[2])
				src[2]--;
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

size_t ReadVoxel(FILE* f, uint8_t data, size_t off)
{
	int err;
	fseek(f, off, SEEK_SET);
	if(err = fread(&data, sizeof(uint8_t), 1, f) != 1) 
		return 1;
	return 0;
}

size_t ReadLine(FILE* f, uint8_t *data, size_t cur, size_t LINE, size_t off)
{
	int err;
	fseek(f, off, SEEK_SET);
	if(err = fread(&data[cur], sizeof(uint8_t), LINE, f) != LINE)
		return 1;
	cur+=LINE;
	return 0;
}

size_t ReadBrick(FILE* f, uint8_t* data, size_t GBSIZE, size_t GDIM, size_t cur)
{
	int err;
	for(size_t z=0; z<GBSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			fseek(f, cur, SEEK_SET);
			if(err = fread(&data[cur], sizeof(uint8_t), GBSIZE, f) != GBSIZE)
				printf("ERROR");
			printf("%d %d\n",data[cur], cur);
			cur+=GBSIZE+4*GDIM;
		}
	}
}

size_t GetBricks(FILE* f, uint8_t* data, size_t GBSIZE, size_t GDIM, size_t b[3], size_t bpd[3])
{
	size_t cur = 0;
	for(size_t z=0; z<2; z++) {
		for(size_t y=0; y<2; y++) {
			for(size_t x=0; x<2; x++) {
				size_t bc[3] = {x+b[0],y+b[1],z+b[2]}; 
				fseek(f, GetBrickId(bc,bpd)*GBSIZE*GBSIZE*GBSIZE, SEEK_SET);
				printf("==== %d %d %d %d %d\n",cur, x,y,z,GetBrickId(bc,bpd)*GBSIZE*GBSIZE*GBSIZE);
				ReadBrick(f, data, GBSIZE, GDIM, cur);	
				printf("==== %d %d %d %d %d\n",cur, x,y,z,GetBrickId(bc,bpd)*GBSIZE*GBSIZE*GBSIZE);			
				cur+=GDIM*GBSIZE*GBSIZE+GDIM*GBSIZE+GDIM+GBSIZE*GBSIZE*GBSIZE;
			}
		}	
	}
}

void CalcMultiRes(FILE* fpi, FILE* fpo, const size_t BSIZE, const size_t GDIM, size_t VOLUME[3], size_t fo)
{
	size_t GBSIZE = BSIZE+2*GDIM;
	size_t bpd[3];
	/* how many bricks are in this lod */
	size_t numberofbricks = NBricks(VOLUME, 2, bpd);
	printf("GBSIZE:%d #Bricks%d\n",GBSIZE, numberofbricks);
	size_t bNO = 0;		
	for(size_t i=0; i<numberofbricks; i++) {
		size_t b[3];
		GetBrickCoordinates(b, bpd, bNO);
		uint8_t *data = malloc(sizeof(uint8_t) * BSIZE*BSIZE*BSIZE * 8);
		GetBricks(fpi, data, BSIZE, GDIM, b, VOLUME);
		uint8_t *odata = malloc(sizeof(uint8_t) * BSIZE*BSIZE*BSIZE * 4);
		WriteBrick(fpo, data, GBSIZE*GBSIZE*GBSIZE*8, 0);	
		bNO+=2;
	}
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
	fpo = fopen(fn,"wb");
	if(fpo == NULL) {
		printf("Couldn't open file %s.raw\n", fn);
		return EXIT_FAILURE;
	}
	
	
	
	for(size_t no_b=0; no_b<numberofbricks; no_b++) {
		size_t b[3];
		GetBrickCoordinates(b,bricks_per_dimension,no_b);
		int src[3] = 
			{(b[0]%bricks_per_dimension[0]*BRICKSIZE)-GHOSTCELLDIM,
			 (b[1]%bricks_per_dimension[1]*BRICKSIZE)-GHOSTCELLDIM,
			 (b[2]%bricks_per_dimension[2]*BRICKSIZE)-GHOSTCELLDIM};

		int edge[6] = {0,0,0,0,0,0};
		CheckEdge(b, &edge[0], GHOSTCELLDIM, VOLUME, BRICKSIZE, src, no_b);
		uint8_t *data = malloc(sizeof(uint8_t) * GBSIZE * GBSIZE * GBSIZE);		
		if(ReadFile(fpi, VOLUME, data, src, GBSIZE, edge, no_b) != 0) 
			return EXIT_FAILURE;

		size_t buffer = no_b*GBSIZE*GBSIZE*GBSIZE;
		WriteBrick(fpo, data, GBSIZE*GBSIZE*GBSIZE, buffer);
		free(data);
		//printf("Brick %d done\n", no_b);
	}
	fclose(fpi);
	fclose(fpo);
	fpo = fopen(fn,"rb");

	FILE *fpm = NULL;
	fpm = fopen("xmulti.raw", "w+b");
	if(fpm == NULL) {
		printf("Couldn't open file multi.raw\n");
		return EXIT_FAILURE;
	}
	
	/* offset after every bricking/multires step */
	size_t lod = 1;
	size_t full_offset = 0;
	while(pow(2,lod)<=BRICKSIZE) {

		CalcMultiRes(fpo, fpm, BRICKSIZE, GHOSTCELLDIM, bricks_per_dimension, full_offset);

		lod++;
	}
	
	fclose(fpo);
	fclose(fpm);	
	return EXIT_SUCCESS;
}
