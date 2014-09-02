#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <mpi.h>

/* needs to go into rw.c */
size_t ReadAndAdd(MPI_File fpo, const size_t ADD_FROM, const size_t LINE) {
	size_t n = LINE;
	uint8_t tmp[n];
	size_t out=0;
	MPI_File_seek(fpo, +ADD_FROM, MPI_SEEK_SET);
	MPI_File_read(fpo, &tmp, n, MPI_UINT8_T, MPI_STATUS_IGNORE);
	for(size_t i=0; i<n; i++)  {
		out += tmp[i];
		//printf("%d\n",tmp[i]);
	}

	return out;
}

/* fo: offset of all previous hierarchies (local)
  NFIELDS: number of "collapsing cells" in one dimension (local)
  goff: offset of the BRICK in the volume (global)
  loff: offset of "collapsing cells" INSIDE the BRICK (local)
  bpd: bricks per dimension (global) */
size_t GetOffset(size_t fo, size_t goff[3], size_t loff[3], const size_t NFIELDS, size_t bpd[3]) 
{
	return fo+goff[0]*NFIELDS+goff[1]*NFIELDS*bpd[0]*NFIELDS+goff[2]*NFIELDS*bpd[0]*NFIELDS*bpd[1]*NFIELDS+loff[0]+loff[1]*NFIELDS*bpd[0]+loff[2]*NFIELDS*bpd[0]*NFIELDS*bpd[1];
}

/* fpo: MPI_File of bricked volume (read)
  fpm: MPI_File of multires (write)
  brick_offset: offset in the bricked volume 
  edge: is the brick on the left edge? (global)
  brick_number: number of brick, starts left top in the bricked volume with 0
  bricks_per_dim: how many bricks are there in one dimension */
void CalcMultiresolutionHierarchy(MPI_File fpo, MPI_File fpm, size_t brick_offset, const size_t GBSIZE, const size_t BSIZE, const size_t GHOSTCELLDIM, bool edge, size_t brick_number, size_t bricks_per_dim[3], int rank) 
{
	/* current # of "collapsing" voxels */
	size_t current = 2;
	/* offset of all previous hierarchies (local) */
	size_t fulloffset = 0;
	size_t numberofbricks = BSIZE*BSIZE*BSIZE/current/current/current;	
	while(current < 3) {
		/*   GLOBAL offset of the BRICK in the volume*/
		size_t goffset[3] = {0,0,0};
		for(size_t i=0; i<brick_number; i++) {
			goffset[0]++;
			if(goffset[0] >= bricks_per_dim[0]) {
				goffset[1]++;
				goffset[0]=0;
				if(goffset[1] >= bricks_per_dim[1]) {
					goffset[2]++;	
					goffset[1]=0;
				}
			}
		}
		printf("RANK: %d, brick number %d, brick offset %d, %d %d %d, \n", rank, brick_number,brick_offset, goffset[0], goffset[1], goffset[2] );
		/* number of "collapsing cells or fields" in one dimension */
		size_t NFIELDS = BSIZE/current;
		/* LOCAL offset INSIDE the BRICK */
		size_t offset[3] = { 0,0,0 };

		for(size_t no_b=0; no_b < numberofbricks; no_b++) {
			size_t coffset[3] = { offset[0], offset[1] + GHOSTCELLDIM, offset[2] + GHOSTCELLDIM};
			if(edge == true)
				coffset[0] += GHOSTCELLDIM;
			size_t start = coffset[1];
			size_t sum = 0;
			for(size_t z=0; z<current; z++) {
				for(size_t y=0; y<current; y++) {
					const size_t ADD_FROM = brick_offset+(coffset[0]+coffset[1]*GBSIZE+coffset[2]*GBSIZE*GBSIZE);
					sum+=ReadAndAdd(fpo, ADD_FROM, current);
					coffset[1]++;
		 		}
				coffset[1] = start;
				coffset[2]++;
			}
			/* get average */
			sum/=(current*current*current);

			/* LOCAL offset divided by current INSIDE the BRICK
			  needed for the calculation of the offset for writing output */
			size_t loffset[3] = {offset[0]/current, offset[1]/current, offset[2]/current};
			
			const size_t MYOFFSET = GetOffset(fulloffset, goffset, loffset, NFIELDS, bricks_per_dim);
			MPI_File_seek(fpm, MYOFFSET, MPI_SEEK_SET);
			MPI_File_write(fpm, &sum, 1, MPI_UINT8_T, MPI_STATUS_IGNORE);		
			offset[0]+=current;
			if(offset[0] >= BSIZE) {
				offset[1]+=current;
				if(offset[1] >= BSIZE) {
					offset[1] = 0;
					offset[2]+=current;
				}
				offset[0] = 0;
			}
		}	
		fulloffset+=numberofbricks*bricks_per_dim[0]*bricks_per_dim[1]*bricks_per_dim[2];
		current*=2;			
	}

}
