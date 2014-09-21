#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <mpi.h>


void getOffsets(size_t *o, size_t *b, const size_t VOLUME[3], const size_t BS, size_t size) 
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
/* how many bricks are needed? */
size_t NBricks(size_t VOLUME[3], const size_t BRICKSIZE, size_t *bdim)
{
	if(VOLUME[0] < BRICKSIZE || VOLUME[1] < BRICKSIZE || VOLUME[2] < BRICKSIZE)
	return 0;
	for(size_t i=0; i<3; i++) {
	bdim[i] = VOLUME[i] / BRICKSIZE;
	if(VOLUME[i] % BRICKSIZE != 0) {
		bdim[i]++;
	}
}
	return bdim[0]*bdim[1]*bdim[2];
}

/* right, bot, back */
void CheckEdge(size_t b[3], int edge[6], const size_t GDIM, size_t VOLUME[3], const size_t BRICKSIZE, int src[3], size_t bno) {
	/* left top front */
	for(size_t i=0; i<3; i++) {
		if(src[i]<0) {
			edge[i*2] = (src[i])*-1;
			src[i]=0;

		}
	}
	/* right bot back */
	for(size_t i=0; i<3; i++) {
		if(b[i]+GDIM>VOLUME[i]/BRICKSIZE)
			edge[i*2+1]=GDIM;
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
size_t ReadFile(MPI_File f, size_t VOLUME[3], uint8_t *data, int src[3], const size_t GBSIZE, int edge[6], size_t bno)
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
			int a;
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

size_t WriteBrick(MPI_File f, uint8_t *data, size_t length, size_t off)
{
    MPI_File_seek(f, off, MPI_SEEK_SET);
    /*TODO check fwrite*/
    MPI_File_write(f, &data[0], length, MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);
	return 0;
}


size_t ReadLine(MPI_File f, uint8_t *data, size_t cur, size_t LINE, size_t off)
{
    MPI_File_seek(f, off, MPI_SEEK_SET);
	/*TODO check fread*/
    MPI_File_read(f, &data[cur], LINE, MPI_UNSIGNED_CHAR, MPI_STATUS_IGNORE);
	return 0;
}

size_t Get8Bricks(MPI_File fin, MPI_File fout, uint8_t *data, size_t b[3], size_t last_bpd[3], const size_t current_brick, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, size_t lod)
{
    size_t write_offset = 0;
    for(size_t brickZ=0; brickZ<2; brickZ++) {
        for(size_t brickY=0; brickY<2; brickY++) {
            for(size_t brickX=0; brickX<2; brickX++) {
                size_t current_brick[3] = {b[0]*2+brickX, b[1]*2+brickY, b[2]*2+brickZ};
                size_t current_id = GetBrickId(current_brick, last_bpd);
                size_t offset = current_id*GBSIZE*GBSIZE*GBSIZE;
                if(ReadLine(fin, data, write_offset, GBSIZE*GBSIZE*GBSIZE, offset) != 0) {
                    printf("ReadLine error\n");
                    return 1;
                }
                write_offset+=GBSIZE*GBSIZE*GBSIZE;
                //printf("Read %d %d %d\n", current_brick[0],current_brick[1],current_brick[2]);
            }
        }
    }
    return 0;
}

size_t GetGhostcells(MPI_File f, uint8_t *data, const size_t READ_DATA_FROM, uint8_t *ghostcells, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, size_t b[3], size_t id, size_t old_bpd[3], const size_t EDGE[6])
{
    size_t write_at = 0;
    /* front */
    if(EDGE[4]==1&&GDIM>0) {
        if(b[2]>0) {
            size_t new_b[3] = {b[0], b[1], b[2]-1};
            size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-2*GDIM*GBSIZE*GBSIZE;
            size_t line = 2*GDIM*GBSIZE*GBSIZE;
			if(ReadLine(f, ghostcells, write_at, line, offset) != 0)
				printf("error front gc\n");
			write_at+=line;
		} else {
			size_t offset = READ_DATA_FROM+GDIM*GBSIZE*GBSIZE;
			size_t line = GDIM*GBSIZE*GBSIZE;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
		}
	}
	/* back */
	if(EDGE[5]==1 && GDIM>0) {
		if(b[2]+1 < old_bpd[2]) {
			size_t new_b[3] = {b[0], b[1], b[2]+1};
			size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE;
			size_t line = 2*GDIM*GBSIZE*GBSIZE;
			if(ReadLine(f, ghostcells, write_at, line, offset) != 0)
				printf("error back gc\n");
			write_at+=line;
		} else {
			size_t offset = READ_DATA_FROM+GBSIZE*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE-GDIM*GBSIZE*GBSIZE;
			size_t line = GDIM*GBSIZE*GBSIZE;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
			memcpy(&ghostcells[write_at], &data[offset], line);
			write_at+=line;
		}
	}
	for(size_t z=0; z<BSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/* top */
			if(EDGE[2] == 1 && y < GDIM) {
				if(b[1] > 0) {
					size_t new_b[3] = {b[0], b[1]-1, b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE + z*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-2*GDIM*GBSIZE;
					size_t line = 2*GDIM*GBSIZE;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
                        printf("error top gc\n");
                        return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+GDIM*GBSIZE;
					size_t line = GDIM*GBSIZE;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
			/* bottom */
			if(EDGE[3] == 1 && y>=(BSIZE+GDIM)) {
				if(b[1]+1 < old_bpd[1]) {
					size_t new_b[3] = {b[0], b[1]+1, b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+GDIM*GBSIZE;
					size_t line = 2*GDIM*GBSIZE;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
                            printf("%d %d || %d %d %d\n", b[1], old_bpd[1], offset, line, 0);
						printf("error bot gc\n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+GBSIZE*GBSIZE-GDIM*GBSIZE-GBSIZE;
					size_t line = GBSIZE;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
		}
	}
    for(size_t z=0; z<BSIZE; z++) {
		for(size_t y=0; y<GBSIZE; y++) {
			/* left */
			if(EDGE[0] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
				if(b[0] > 0) {
					size_t new_b[3] = {b[0]-1, b[1], b[2]};
					size_t offset = GetBrickId(new_b, old_bpd)*GBSIZE*GBSIZE*GBSIZE+GDIM*GBSIZE*GBSIZE+z*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
					size_t line = 2*GDIM;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error left gc\n");
						return 1;
					}
					write_at+=line;

				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GDIM;
					size_t line=GDIM;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
			/* right */
			if(EDGE[1] == 1 && y>=GDIM && y<(BSIZE+GDIM)) {
				if(b[0]+1 < old_bpd[0]) {
					size_t ob[3] = {b[0]+1, b[1], b[2]};
					size_t offset = GetBrickId(ob, old_bpd)*GBSIZE*GBSIZE*GBSIZE+(GDIM+z)*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM-2*GDIM;
					size_t line = 2*GDIM;
					if(ReadLine(f, ghostcells, write_at, line, offset) != 0) {
						printf("error right gc \n");
						return 1;
					}
					write_at+=line;
				} else {
					size_t offset = READ_DATA_FROM+(z+GDIM)*GBSIZE*GBSIZE+y*GBSIZE+GBSIZE-GDIM;
					size_t line = GDIM;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
					memcpy(&ghostcells[write_at], &data[offset], line);
					write_at+=line;
				}
			}
		}
	}

    return 0;
}

void ReorganiseArray(uint8_t *data, uint8_t *ghostcells, uint8_t *finalData, const size_t READ_DATA_FROM, const size_t WRITE_FINAL_DATA_AT, const size_t BSIZE, const size_t GDIM, const size_t GBSIZE, const size_t EDGE[6], size_t b[3])
{
    const size_t ID = b[0]+b[1]*2+b[2]*4;
    size_t read_data_at = GDIM*GBSIZE*GBSIZE+GDIM*GBSIZE+GDIM;
    size_t helper = ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4;
    size_t front[4] = {0,GBSIZE,2*GBSIZE*GBSIZE,2*GBSIZE*GBSIZE+GBSIZE};
    size_t back[4] = {2*helper-8*GDIM*GBSIZE*GBSIZE, 2*helper-8*GDIM*GBSIZE*GBSIZE+GBSIZE, 2*helper-8*GDIM*GBSIZE*GBSIZE+2*GBSIZE*GBSIZE, 2*helper-8*GDIM*GBSIZE*GBSIZE+2*GBSIZE*GBSIZE+GBSIZE};
    size_t top[8] =
        {4*2*GDIM*GBSIZE*GBSIZE,
        4*2*GDIM*GBSIZE*GBSIZE+GBSIZE,
        4*2*GDIM*GBSIZE*GBSIZE+4*GDIM*GBSIZE+4*GBSIZE*BSIZE,
        4*2*GDIM*GBSIZE*GBSIZE+4*GDIM*GBSIZE+4*BSIZE*GBSIZE+GBSIZE,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+GBSIZE,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+ 4*GDIM*GBSIZE+4*GBSIZE*BSIZE,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+ 4*GDIM*GBSIZE+4*GBSIZE*BSIZE+GBSIZE};
    size_t left[8] = {
        top[0]+2*(GBSIZE*2*GDIM),
        top[0]+2*(GBSIZE*2*GDIM)+2*GDIM+2*BSIZE,
        top[0]+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE),
        top[0]+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE)+2*GDIM+2*BSIZE,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM),
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+2*GDIM+2*BSIZE,
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE),
        ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*4+2*(GBSIZE*2*GDIM)+BSIZE*(2*2*GDIM+2*BSIZE)+2*GDIM+2*BSIZE
        };
    size_t data_start[8] = {
        left[0]+2*GDIM,
        left[0]+2*GDIM+BSIZE,
        left[2]+2*GDIM,
        left[2]+2*GDIM+BSIZE,
        left[4]+2*GDIM,
        left[4]+2*GDIM+BSIZE,
        left[6]+2*GDIM,
        left[6]+2*GDIM+BSIZE};

    for(size_t z=0 ;z<2*GDIM; z++) {
        for(size_t y=0; y<GBSIZE; y++) {
                /*FRONT*/
                if(EDGE[4]==1) {
                    memcpy(&finalData[front[ID]+z*4*GBSIZE*GBSIZE+y*2*GBSIZE], &ghostcells[z*GBSIZE*GBSIZE+y*GBSIZE], GBSIZE);
                }
                /*BACK*/
                if(EDGE[5]==1) {
                    memcpy(&finalData[back[ID-4]+z*4*GBSIZE*GBSIZE+y*2*GBSIZE], &ghostcells[z*GBSIZE*GBSIZE+y*GBSIZE], GBSIZE);
                }
        }
    }
    for(size_t z=0; z<BSIZE; z++) {
        for(size_t y=0; y<GBSIZE+2*GDIM; y++) {
                /* TOP && BOTTOM */
            if((EDGE[2]==1 || EDGE[3]==1) && y<2*GDIM) {
                memcpy(&finalData[top[ID]+z*4*GBSIZE*GBSIZE+y*2*GDIM*GBSIZE], &ghostcells[2*GDIM*GBSIZE*GBSIZE+z*2*GDIM*GBSIZE+y*GDIM*GBSIZE], GBSIZE);
            }
            if((EDGE[0]==1 || EDGE[1]==1) && y>=2*GDIM && y<2*GDIM+BSIZE) {
                memcpy(&finalData[left[ID]+z*((2*2*GDIM+2*BSIZE)*2*BSIZE+2*(2*2*GDIM*GBSIZE))+(y-2*GDIM)*2*GDIM*GBSIZE], &ghostcells[2*GDIM*GBSIZE*GBSIZE+2*GDIM*GBSIZE*BSIZE+z*2*GDIM*2*GDIM+(y-2*GDIM)*2*GDIM], 2*GDIM);
            }
            if(y>=2*GDIM && y<2*GDIM+BSIZE) {
                memcpy(&finalData[data_start[ID]+z*((2*2*GDIM+2*BSIZE)*2*BSIZE+2*(2*2*GDIM*GBSIZE))+(y-2*GDIM)*2*GDIM*GBSIZE], &data[ID*GBSIZE*GBSIZE*GBSIZE+(z+GDIM)*GBSIZE*GBSIZE+(y+GDIM-2*GDIM)*GBSIZE+GDIM], BSIZE);
            }
        }
    }

/*
    size_t ct=0;
    for(size_t i=0; i<(((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*8);i++) {
            if(finalData[i]<10) printf("0");
        printf("%d ", finalData[i]);
        ct++;
        if(ct==8) {
            ct=0;
            printf("\n");
        }
    }
    printf("\n");
    */
}

uint8_t CollapseVoxels(uint8_t *data, size_t off, size_t b[3], const size_t GBSIZE)
{
	size_t out = 0;
	size_t o = b[0]+b[1]*GBSIZE*2+b[2]*GBSIZE*2*GBSIZE*2;
	//printf("\n>>>>>>>>>>>>> %d %d %d %d\n", o, b[0],b[1],b[2]);
	for(size_t z=0; z<2; z++) {
	for(size_t y=0; y<2; y++) {
	    for(size_t x=0; x<2; x++) {
		out+=data[o+x+y*GBSIZE+z*GBSIZE*GBSIZE];
		//printf("%d ",data[o+x+y*GBSIZE+z*GBSIZE*GBSIZE]);
	    }
	}
	}
	//printf("\n%d %d %d : %d\n",b[0],b[1],b[2],out/8);
	return out/8;
}

size_t LowerLOD(MPI_File f, uint8_t *data, const size_t GBSIZE, size_t GDIM, size_t output_offset)
{
    for(size_t z=0; z<GBSIZE; z++) {
        for(size_t y=0; y<GBSIZE; y++) {
            for(size_t x=0; x<GBSIZE; x++) {
            size_t small_b[3] = {x*2,y*2,z*2};
            uint8_t out[1] = {CollapseVoxels(data, 0, small_b, GBSIZE)};
            if(WriteBrick(f, out, 1, output_offset) != 0)
                return 1;
            output_offset++;
            }
        }
    }
    return 0;
}


size_t GetNewLOD(MPI_File fin, MPI_File fout, size_t old_bpd[3], size_t VOLUME[3], size_t current_bpd[3], size_t current_no_bricks, const size_t BSIZE, const size_t GDIM, size_t lod, int rank, int size)
{
	const size_t GBSIZE=BSIZE+2*GDIM;
	size_t last_bpd[3] = {old_bpd[0], old_bpd[1], old_bpd[2]};
	for(size_t i=0; i<lod-1;i++) {
		NBricks(last_bpd, 2, last_bpd);
	}	
	size_t mybricks, mystart;
	size_t *starting_brick = malloc(sizeof(size_t)*size);
	size_t *bricks = malloc(sizeof(size_t)*size);
	if(rank==0) {
		getBricks(bricks, starting_brick, size, current_no_bricks);
		mybricks=bricks[0];
		mystart=starting_brick[0];
		if(size>1) {
			for(size_t i=1; i<size; i++) {
	  			if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
				    fprintf(stderr, "ERROR sending # of blocks\n");
				    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
				if(MPI_Send(&starting_brick[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
				    fprintf(stderr, "ERROR sending starting brick\n");
				    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				}
			}
		}
	} else {
		int count;
		MPI_Status stat;
		MPI_Recv(&mybricks, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
		    fprintf(stderr, "Rank %d: ERROR receiving # of bricks\n", rank);
		    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
		MPI_Recv(&mystart, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
		    fprintf(stderr, "Rank %d: ERROR receiving starting brick\n", rank);
		    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
	}
	
	printf("%d %d %d || %d %d %d \n", old_bpd[0], old_bpd[0], old_bpd[1], last_bpd[2], last_bpd[1], last_bpd[2]);	
	printf("Rank %d / Start %d / Bricks %d\n", rank, mystart, mybricks);
	/* for each new brick */
	for(size_t no_b=mystart; no_b < mystart+mybricks; no_b++) {
		size_t b[3];
		GetBrickCoordinates(b, current_bpd, no_b);
		uint8_t *data = malloc(sizeof(uint8_t)*GBSIZE*GBSIZE*GBSIZE*8);
		if(Get8Bricks(fin, fout, data, b, last_bpd, no_b, BSIZE, GDIM, GBSIZE, lod) != 0) {
			printf("Error@Get8Bricks()\n");
			return 1;
		}
		uint8_t *finalData = malloc(sizeof(uint8_t) * ((GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE)*8);
		for(size_t brickZ=0; brickZ<2; brickZ++) {
		    for(size_t brickY=0; brickY<2; brickY++) {
			for(size_t brickX=0; brickX<2; brickX++) {
			    const size_t EDGE[6] = {(brickX+1)%2, brickX%2, (brickY+1)%2, brickY%2, (brickZ+1)%2, brickZ%2};
			    const size_t READ_DATA_FROM = (brickX+brickY*2+brickZ*4)*GBSIZE*GBSIZE*GBSIZE;
			    //printf("%d %d %d :: %d\n", brickX, brickY, brickZ, READ_DATA_FROM);
			    const size_t FINAL_DATA_START_AT = (brickX+brickY*2+brickZ*4)*(GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM+BSIZE*BSIZE*BSIZE;
			    uint8_t *ghostcells = malloc(sizeof(uint8_t)*(GBSIZE*GBSIZE+GBSIZE*BSIZE+BSIZE*BSIZE)*2*GDIM);
			    size_t current_brick[3] = {brickX+b[0]*2,brickY+b[1]*2,brickZ+b[2]*2};
			    size_t current_id = GetBrickId(current_brick, old_bpd);
			    if(GetGhostcells(fin, data, READ_DATA_FROM, ghostcells, BSIZE, GDIM, GBSIZE, current_brick, current_id, last_bpd, EDGE) != 0) {
				printf("GetGhostcells error\n");
				return 1;
			    }
			    size_t b[3] = {brickX, brickY, brickZ};
			    ReorganiseArray(data, ghostcells, finalData, READ_DATA_FROM, FINAL_DATA_START_AT, BSIZE, GDIM, GBSIZE, EDGE, b);
			}
		    }
		}
		LowerLOD(fout, finalData, GBSIZE, GDIM, no_b*GBSIZE*GBSIZE*GBSIZE);
	}

	return 0;
}


int main(int argc, char* argv[])
{

	MPI_Init(&argc,&argv);
	int size,rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(argc!=7) {
		printf("Check input\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);	}
	char *endptr = NULL;
	/* Size of the volume {x,y,z} */
	size_t *VOLUME = malloc(sizeof(size_t)*3);
	VOLUME[0]=strtol(argv[1], &endptr, 10);
	VOLUME[1]=strtol(argv[2], &endptr, 10);
	VOLUME[2]=strtol(argv[3], &endptr, 10);
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

	size_t *myoffsets = malloc(sizeof(size_t)*3);
	size_t mybricks;
	size_t *offsets;
	size_t bricks[size];
	size_t starting_brick[size];
	size_t mystart;
	size_t GBSIZE=BRICKSIZE+2*GHOSTCELLDIM;

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
			/* TODO why wont this take length==3?? */
		        if(MPI_Send(&offsets[i*3+0], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
		            fprintf(stderr, "ERROR sending offset\n");
		            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		        }
		        if(MPI_Send(&offsets[i*3+1], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
		            fprintf(stderr, "ERROR sending offset\n");
		            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		        }
		        if(MPI_Send(&offsets[i*3+2], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
		            fprintf(stderr, "ERROR sending offset\n");
		            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		        }
		        if(MPI_Send(&bricks[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
		            fprintf(stderr, "ERROR sending # of blocks\n");
		            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		        }
		        if(MPI_Send(&starting_brick[i], 1, MPI_UNSIGNED, i, 42, MPI_COMM_WORLD) != MPI_SUCCESS) {
		            fprintf(stderr, "ERROR sending starting brick\n");
		            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		        }
		    }
		}
	} else {
		MPI_Status stat;
		int count;
		/* TODO why wont this take length==3??? */
		MPI_Recv(&myoffsets[0], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Recv(&myoffsets[1], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Recv(&myoffsets[2], 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
		    fprintf(stderr, "Rank %d: ERROR receiving offset\n", rank);
		    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
		MPI_Recv(&mybricks, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
		    fprintf(stderr, "Rank %d: ERROR receiving # of bricks\n", rank);
		    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
		MPI_Recv(&mystart, 1, MPI_UNSIGNED, 0, 42, MPI_COMM_WORLD, &stat);
		MPI_Get_count(&stat, MPI_UNSIGNED, &count);
		if(count!=1) {
		    fprintf(stderr, "Rank %d: ERROR receiving starting brick\n", rank);
		    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		}
	}
	printf("RANK: %d BRICKS: %d OFFSET_X %d OFFSET_Y %d OFFSET_Z %d\n", rank, mybricks, myoffsets[0], myoffsets[1], myoffsets[2]);
	/* local offsets for every rank */
	size_t offset[3] = {myoffsets[0],myoffsets[1],myoffsets[2]};
	size_t orig_offset[3] = {offset[0],offset[1],offset[2]};
	
	printf("Bricksize %d || GBricksize %d\n", BRICKSIZE, GBSIZE);


	/* TODO check fopen */

	/* input file stream */
	MPI_File fpi;
	MPI_File_open(MPI_COMM_WORLD, argv[6], MPI_MODE_RDONLY, MPI_INFO_NULL, &fpi);

	/* output file stream */
	MPI_File fpo;
	char fn[256];
	sprintf(fn, "b_%d_%d_%d_%d^3.raw", VOLUME[0], VOLUME[1], VOLUME[2], GBSIZE);
	int err;
	err = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fpo);
    	if(err) printf("ERROR opening file %s\n",fn);	

	size_t fdataoffset = bricks_per_dimension[0] * GBSIZE * bricks_per_dimension[1] * GBSIZE * bricks_per_dimension[2] * GBSIZE;
	//printf("%d %d %d %d\n", fdataoffset, bricks_per_dimension[0],bricks_per_dimension[1],bricks_per_dimension[2]);
	for(size_t no_b=mystart; no_b<mystart+mybricks; no_b++) {

          /* offset for output file */
        int output_file_offset = no_b*GBSIZE*GBSIZE*GBSIZE;
        MPI_File_seek(fpo, output_file_offset, MPI_SEEK_SET);
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
	/* TODO check if offset correct */
	size_t offset = no_b*GBSIZE*GBSIZE*GBSIZE;
	WriteBrick(fpo, data, GBSIZE*GBSIZE*GBSIZE, offset);
	free(data);
	//printf("Brick %d done\n", no_b);
	}
	printf("RANK %d has just finished bricking\n", rank);
	MPI_File_close(&fpi);
	MPI_File_close(&fpo);

	size_t lod = 1;
	/* TODO set finished correct */
	bool finished = false;
	while(!finished) {
		/* read from */
		MPI_File fin;
		MPI_File fout;
	    	if(lod==1) {
		    	err = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
		    	if(err) printf("ERROR opening file %s\n",fn);
		    	printf("File %s opened\n", fn);
	    	} else {
			char filename_read[256];
			sprintf(filename_read, "xmulti_%d.raw", lod-1);
			err = MPI_File_open(MPI_COMM_WORLD, filename_read, MPI_MODE_RDONLY, MPI_INFO_NULL, &fin);
			if(err) printf("ERROR opening file %s\n",filename_read);
			printf("File %s opened\n", filename_read);
		}
		/* write to */
		char filename_write[256];
		sprintf(filename_write, "xmulti_%d.raw", lod);
		err = MPI_File_open(MPI_COMM_WORLD, filename_write, MPI_MODE_WRONLY | MPI_MODE_CREATE , MPI_INFO_NULL, &fout);
	    	if(err) printf("ERROR opening file %s\n",filename_write);	
		printf("File %s opened\n", filename_write);
		MPI_Barrier(MPI_COMM_WORLD);
		if(rank==0)
		    printf("### LOD Level %d ###\n", lod);
		/* TODO offsets */
		size_t new_no_b=0;
		size_t new_bpd[3] = {0,0,0};
		size_t old_bpd[3] = {bricks_per_dimension[0],bricks_per_dimension[1],bricks_per_dimension[2]};
		for(size_t i=0; i<lod; i++) {
			new_no_b = NBricks(old_bpd, 2, new_bpd);
			old_bpd[0] = new_bpd[0];
			old_bpd[1] = new_bpd[1];
			old_bpd[2] = new_bpd[2];
		}
		printf("%d %d || %d %d %d\n",new_no_b, 42, new_bpd[0],new_bpd[1],new_bpd[2]);
		GetNewLOD(fin, fout, bricks_per_dimension, VOLUME, new_bpd, new_no_b,BRICKSIZE,GHOSTCELLDIM,lod,rank,size);
		lod++;
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_File_close(&fin);
		MPI_File_close(&fout);
		if(new_no_b<2)
			finished=true;
	}

	MPI_Finalize();
   	return EXIT_SUCCESS;
}
