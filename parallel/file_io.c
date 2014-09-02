#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

/* rw = read/write 0/1 */
size_t OpenFile(MPI_File fp, char *fn, size_t rw)
{
	int rc = 42;
	if(rw==0)
		rc = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDONLY, MPI_INFO_NULL, &fp);
	else if(rw==1)
		rc = MPI_File_open(MPI_COMM_WORLD, fn,  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fp);
	else
		rc = MPI_File_open(MPI_COMM_WORLD, fn, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &fp);
	if(rc != MPI_SUCCESS) 
		return 1;
	return 0;
}

