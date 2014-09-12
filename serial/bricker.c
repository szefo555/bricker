#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "rw.h"
#include "multires.h"



/* right, bot, back */
void CheckEdge(size_t b[3], int edge[6], const size_t GDIM, const size_t VOLUME[3], const size_t BRICKSIZE, int src[3], size_t bno) {
	/* left top front */
	for(size_t i=0; i<3; i++) {
		if(src[i]<0) {
			edge[i*2] = (src[i])*-1;
			src[i]=0;
			
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

/* edge2 right, bot, back */
size_t ReadFile(FILE *f, const size_t VOLUME[3], uint8_t *data, int src[3], const size_t GBSIZE, int edge[6], size_t bno)
{
	size_t cur = 0;
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
			size_t LINE = GBSIZE-edge[0]-edge[1];
			while(src[0]+LINE>VOLUME[0])
				LINE--;
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


size_t ReadLine(FILE* f, uint8_t *data, size_t cur, size_t LINE, size_t off)
{
	int err;
	fseek(f, off, SEEK_SET);
	if(err = fread(&data[cur], sizeof(uint8_t), LINE, f) != LINE) {
		printf("Read %d out of %d\n",err, LINE);
		return 1;
	}
	return 0;
}

size_t GetNumberOfVoxels(size_t bpd[3], const size_t GBSIZE, const size_t GDIM)
{
	return 8*GBSIZE*GBSIZE*GBSIZE; /*for now*/
}

size_t GetBricks(FILE* f, uint8_t *data, size_t b[3], size_t bpd[3], const size_t BSIZE, const size_t GDIM, const size_t GBSIZE)
{
	size_t cur = 0;
	size_t fo = 0; /* NEEDS FULL OFFSET */
	for(size_t brickZ=0; brickZ<2; brickZ++) {
		for(size_t brickY=0; brickY<2; brickY++) {
			for(size_t brickX=0; brickX<2; brickX++) {
				size_t edge[6] = {(brickX+1)%2, brickX%2, (brickY+1)%2, brickY%2, (brickZ+1)%2, brickZ%2};
				size_t current_brick[3] = {b[0]*2+brickX, b[1]*2+brickY, b[2]*2+brickZ};
				size_t current_id = GetBrickId(current_brick, bpd);
				/*printf("%d || %d %d %d || %d %d %d %d %d %d || %d %d %d\n", current_id, current_brick[0],current_brick[1],current_brick[2],edge[0],edge[1],edge[2],edge[3],edge[4],edge[5], bpd[0], bpd[1], bpd[2]); */
				/* front */
				if(edge[4] == 1 && GDIM > 0) {
					/* ghostcells */
					if(current_brick[2] > 0) {
						size_t ob[3] = {current_brick[0], current_brick[1],current_brick[2]-1};
						size_t offset = fo + GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE + GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-2*GDIM*GBSIZE*GBSIZE;
						size_t line = 2*GDIM*GBSIZE*GBSIZE;
						if(ReadLine(f, data, cur, line, offset) != 0)
							printf("error front gc\n");
						cur+=line;
					} else {
						size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE;
						size_t line = GDIM*GBSIZE*GBSIZE;
						if(ReadLine(f, data, cur, line, offset) != 0)
							printf("error front no gc\n");
						memcpy(&data[cur+line], &data[cur], line);
						cur+=2*line;
					}
				}
				for(size_t z=0; z<BSIZE; z++) {
					for(size_t y=0; y<GBSIZE; y++) {
						/* top */
						if(edge[2] == 1 && y < GDIM) {
							/* ghostcells */
							if(current_brick[1] > 0) {
								size_t ob[3] = {current_brick[0], current_brick[1]-1, current_brick[2]};
								size_t offset = fo + GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE + z*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-2*GDIM*GBSIZE;
								size_t line = 2*GDIM*GBSIZE;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("error top gc\n");
								cur+=line;	
							} else {
								size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+GDIM*GBSIZE;
								size_t line = GDIM*GBSIZE;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("error top no gc\n");
								memcpy(&data[cur+line], &data[cur], line);
							}
						}
						/* left */
						if(edge[0] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
							if(current_brick[0] > 0) {
								size_t ob[3] = {current_brick[0]-1, current_brick[1], current_brick[2]};
								size_t offset = fo + GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE + GDIM*GBSIZE*GBSIZE+z*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
								size_t line = 2*GDIM;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("error left gc\n");
								cur+=line;
								
							} else {
								size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GDIM;
								if(ReadLine(f, data, cur, 1, offset) != 0)
									printf("error left no gc\n");
								for(size_t i=0; i<2*GDIM; i++) 
									data[cur+i] = data[cur];
								cur+=2*GDIM;
									
							}
						}
						/* finally real data */
						if(y>=GDIM && y<(GDIM+BSIZE)) {
						size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GDIM;
						if(ReadLine(f, data, cur, BSIZE, offset) != 0)
							printf("error reading 'real data'\n");
						cur+=BSIZE;
						}
						/* right */
						if(edge[1] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
							if(current_brick[0]+1 < bpd[0]) {
								size_t ob[3] = {current_brick[0]+1, current_brick[1], current_brick[2]};
								size_t offset = fo+GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE+(GDIM+z)*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
								size_t line = 2*GDIM;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("error right gc \n");
								cur+=line;									
							} else {
								size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GDIM;
								if(ReadLine(f, data, cur, 1, offset) != 0)
									printf("error right no gc\n");
								for(size_t i=0; i<2*GDIM; i++) 
									data[cur+i] = data[cur];
								cur+=2*GDIM;	
							}
						}
						/* bot */
						if(edge[3] == 1 && y>=(GBSIZE+GDIM)) {
							if(current_brick[1]+1 < bpd[1]) {
								size_t ob[3] = {current_brick[0], current_brick[1]+1, current_brick[2]};
								size_t offset = fo+GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE+(z+GDIM) *GBSIZE*GBSIZE+GDIM*GBSIZE;
								size_t line = 2*GDIM*GBSIZE;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("bot gc error\n");
								cur+=line;
								
							} else {
								size_t offset = fo+current_id*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-GBSIZE;
								size_t line = GBSIZE;
								if(ReadLine(f, data, cur, line, offset) != 0)
									printf("bot no gc error\n");
								for(size_t i=0; i<2*GDIM-line; i++) 
									data[cur+line+i] = data[cur];
								cur+=2*GDIM;
							}
						}						
					}
				}
				/* back */
				if(edge[5] == 1 && GDIM > 0) {
					/* ghostcells */
					if(current_brick[2]+1 < bpd[2]) {
						size_t ob[3] = {current_brick[0], current_brick[1],current_brick[2]+1};
						size_t offset = fo + GetBrickId(ob, bpd)*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE;
						size_t line = 2*GDIM*GBSIZE*GBSIZE;
						if(ReadLine(f, data, cur, line, offset) != 0)
							printf("error back gc\n");
						cur+=line;
					} else {
						size_t offset = fo + current_id*GBSIZE*GBSIZE*GBSIZE+GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-GBSIZE*GBSIZE;
						size_t line = GBSIZE*GBSIZE;
						if(ReadLine(f, data, cur, line, offset) != 0)
							printf("error back no gc\n");
						memcpy(&data[cur+line], &data[cur], line);
						cur+=2*line;
					}
				}
			}
		}
	}		
	return 0;
}

size_t GetNewLOD(FILE* fin, FILE* fout, size_t ORIG_BPD[3], const size_t ORIG_VOLUME[3], size_t cur_bpd[3], size_t cur_bricks, const size_t BSIZE, const size_t GDIM, size_t lod)
{
	const size_t GBSIZE=BSIZE+2*GDIM;
	size_t old_bpd[3];
	NBricks(ORIG_VOLUME,BSIZE,old_bpd);
	for(size_t i=0; i<lod-1;i++) {
		NBricks(old_bpd, 2, old_bpd);
	}
	printf("%d\n",cur_bricks);
	for(size_t no_b=0; no_b < cur_bricks; no_b++) {
		size_t b[3];
		GetBrickCoordinates(b, cur_bpd, no_b);
		uint8_t *data = malloc(sizeof(uint8_t) * GetNumberOfVoxels(ORIG_BPD,GBSIZE,GDIM));
		if(GetBricks(fin, data, b, old_bpd, BSIZE, GDIM, GBSIZE) != 0) {
			printf("Error@GetBricks()\n");
			return 1;
		}
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
	bool finished = false;
	int ct=0;
	while(!finished) {
		printf("### LOD Level %d ###\n", lod);
		size_t new_no_b=0; 
		size_t new_bpd[3] = {0,0,0};
		size_t old_bpd[3] = {bricks_per_dimension[0],bricks_per_dimension[1],bricks_per_dimension[2]}; 
		for(size_t i=0; i<lod; i++) {
			new_no_b = NBricks(old_bpd, 2, new_bpd);
			old_bpd[0] = new_bpd[0];
			old_bpd[1] = new_bpd[1];
			old_bpd[2] = new_bpd[2];
		}
		//printf("==%d %d\n",new_no_b, ct++);
		GetNewLOD(fpo, fpm, bricks_per_dimension, VOLUME, new_bpd, new_no_b,BRICKSIZE,GHOSTCELLDIM,lod);	
		lod++;
		if(new_no_b<2)
			finished=true;
	}
	
	fclose(fpo);
	fclose(fpm);	
	return EXIT_SUCCESS;
}
