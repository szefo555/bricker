#ifndef misc_h
#define misc_h

size_t NBricks(size_t*, const size_t, size_t*);
void CheckEdge(size_t*, int*, const size_t, size_t*, const size_t, int*);
void GetBrickCoordinates(size_t*, size_t*, size_t);
size_t GetBrickId(size_t* , size_t*);
size_t WriteBrick(MPI_File, uint8_t*, size_t, size_t);
size_t ReadLine(MPI_File, uint8_t*, size_t, size_t, size_t);
#endif
