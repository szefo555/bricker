#ifndef brick_h
#define brick_h

size_t ReadFile(MPI_File, size_t*, uint8_t*, int*, const size_t, int*);
size_t brick(MPI_File, MPI_File, size_t, size_t, const size_t, const size_t, const size_t, size_t*, size_t*);


#endif
