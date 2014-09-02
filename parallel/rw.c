#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <mpi.h>


/* Pad a full line if Y is out of bounds
   Pad a full page if Z is out of bounds */

void Seek(MPI_File fp, int o)
{
	MPI_File_seek(fp, o, MPI_SEEK_SET);
}

void Padding(MPI_File fp, const size_t LINE) 
{
	size_t zero[LINE];
	for(size_t i=0; i<LINE; i++)
		zero[i] = 0;
	MPI_File_write(fp, &zero[0], LINE, MPI_UINT8_T, MPI_STATUS_IGNORE);
}


/* Copy data, pad if necessary (scanline) */
void CopyLine(MPI_File fpo, MPI_File fpi, int COPY_FROM, const size_t LINE, int edge[3])
{
	size_t zero = 0;
	size_t n = LINE;
	n -= edge[1];
	uint8_t tmp[n-edge[0]];
	for(int i=edge[1]; i>0; i--) {
		MPI_File_write(fpo, &zero, 1, MPI_UINT8_T, MPI_STATUS_IGNORE);
	}
	MPI_File_seek(fpi, COPY_FROM, MPI_SEEK_SET);
	MPI_File_read(fpi, &tmp[0], n-edge[0], MPI_UINT8_T, MPI_STATUS_IGNORE);
	MPI_File_write(fpo, &tmp[0], n-edge[0], MPI_UINT8_T, MPI_STATUS_IGNORE);
	for(int i=n-edge[0]; i<n; i++) {
		MPI_File_write(fpo, &zero, 1, MPI_UINT8_T, MPI_STATUS_IGNORE);
	}	
} 
