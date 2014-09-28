#ifndef mpi_init_h
#define mpi_init_h

void getOffsets(size_t*, size_t*, const size_t *VOLUME, const size_t, int);
void getBricks(size_t*, size_t*, size_t, size_t);
size_t Init_MPI(int, int, size_t*, size_t*, size_t*, size_t*, size_t*, size_t, size_t*, const size_t);

#endif
