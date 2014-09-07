#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


/* needs to go into rw.c */
size_t ReadAndAdd(FILE *fpo, const size_t ADD_FROM, size_t *edge) {
	size_t n = 2-edge[0];
	uint8_t tmp[n];
	size_t out=0;
	fseek(fpo, ADD_FROM, SEEK_SET);
	if(fread(&tmp[0], sizeof(uint8_t), n, fpo) == -1337)
		printf("ignore warning\n");
	for(size_t i=0; i<n; i++)  {
		out += tmp[i];
	}
	if(edge[0]==1)
		out+=out;

	return out;
}

void GetGlobalOffset(size_t *goffset, size_t *bpd, size_t brick_number)
{
	for(size_t i=0; i<brick_number; i++) {
		goffset[0]++;
		if(goffset[0] >= bpd[0]) {
			goffset[1]++;
			goffset[0]=0;
			if(goffset[1] >= bpd[1]) {
				goffset[2]++;	
				goffset[1]=0;
			}
		}
	}
}

size_t GetOffset(size_t fo, size_t goff[3], size_t *output, size_t brick_number_out, size_t *bpd) 
{
	return fo+goff[0]*2+goff[1]*2*bpd[0]*2+goff[2]*2*bpd[0]*2*bpd[1]*2+brick_number_out+output[0]+output[1]*2+output[2]*4;
}

void GetCurrentBricksPerDim(size_t *cbpd, size_t *pad, size_t BSIZE)
{
	for(size_t i=0; i<3; i++) {
		cbpd[i] = BSIZE/2;
		if(BSIZE%2!=0) {
			cbpd[i]++;
			pad[i]++;
		}
	} 
}

void CheckEdge(size_t *pad, size_t *offset, size_t *bedge, size_t BSIZE)
{
	for(size_t i=0; i<3; i++) {
		if(pad[i]==1 && offset[i]+2>=BSIZE) 
			bedge[i]++;
	}
}
	
/* fpo: file pointer of bricked volume (read)
  fpm: file pointer of multires (write)
  brick_offset: offset in the bricked volume 
  edge: is the brick on the left edge? (global)
  brick_number: number of brick, starts left top in the bricked volume with 0
  bricks_per_dim: how many bricks are there in one dimension */
void CalcMultiresolutionHierarchy(FILE *fpo, FILE *fpm, size_t brick_offset, const size_t GBSIZE, const size_t BSIZE, const size_t GHOSTCELLDIM, bool edge, size_t brick_number, size_t bricks_per_dim[3]) 
{
	/* current # of "collapsing" voxels */
	size_t current = 2;
	/* offset of all previous hierarchies (local) */
	size_t fulloffset = 0;
	
	while(current<3) {
		if(current==2) {
			size_t goffset[3] = {0,0,0};
			GetGlobalOffset(goffset, bricks_per_dim, brick_number);
			size_t cbpd[3];
			size_t pad[3] = {0,0,0};
			GetCurrentBricksPerDim(cbpd, pad, BSIZE);
			/* number of "collapsing cells or fields" in one dimension */
			size_t NFIELDS = BSIZE/current;
			/* LOCAL offset INSIDE the BRICK */
			size_t offset[3] = { 0,0,0 };
			size_t numberofbricks = cbpd[0]*cbpd[1]*cbpd[2];	

			size_t output[3] = {0,0,0};
			size_t brick_number_out = 0;
			for(size_t no_b=0; no_b < numberofbricks; no_b++) {
				size_t coffset[3] = { offset[0], offset[1] + GHOSTCELLDIM, offset[2] + GHOSTCELLDIM};
				if(edge == true)
					coffset[0] += GHOSTCELLDIM;
				size_t start = coffset[1];
				size_t sum = 0;
				size_t bedge[3] = {0,0,0};
				size_t ct[2] = {0,0};
				CheckEdge(pad, offset, bedge, BSIZE);
				for(size_t z=0; z<2; z++) {
					for(size_t y=0; y<2; y++) {
						const size_t ADD_FROM = brick_offset+(coffset[0]+coffset[1]*GBSIZE+coffset[2]*GBSIZE*GBSIZE);
						sum+=ReadAndAdd(fpo, ADD_FROM, bedge);
						if(bedge[1] != 0) {
							sum+=ReadAndAdd(fpo, ADD_FROM, bedge);
							break;
						}
						coffset[1]++;
		 			}
					if(bedge[2] != 0) {
						sum+=sum;
						break;
					}
					coffset[1] = start;
					coffset[2]++;
				}
				/* get average */
				sum/=8;
			
				const size_t MYOFFSET = GetOffset(fulloffset, goffset, output, brick_number_out, bricks_per_dim);

				fseek(fpm, MYOFFSET, SEEK_SET);
				fwrite(&sum, sizeof(uint8_t), 1, fpm);		
				offset[0]+=current;
				if(offset[0] >= BSIZE) {
					offset[1]+=current;
					if(offset[1] >= BSIZE) {
						offset[1] = 0;
						offset[2]+=current;
					}
					offset[0] = 0;
				}	
				printf("%d: %d | %d %d %d | %d\n", no_b, brick_number_out, cbpd[0],cbpd[1],output[2], MYOFFSET);
				output[0]++;
				if(output[0] > 1) {
					brick_number_out++;
					output[0]=0;
					output[1]++;
					if(output[1] >=cbpd[0]/2) {
						output[1]=0;
						if(ct[0] == 0) {
							brick_number_out--;
							ct[0]++;
						} else {
							ct[0]=0;
						}
						output[2]++;
						if(output[2] >= cbpd[1]) {
							output[2]=0;
							if(ct[1] == 0) {
								brick_number_out-=cbpd[0]*cbpd[1];
								ct[1]++;
							} else {
								ct[1]=0;
							}
						}						
					}
				}
			}
		}
		current*=2;
	}
}
