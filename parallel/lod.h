#ifndef lod_h
#define lod_h

size_t Get8Bricks(MPI_File, uint8_t*, size_t*, size_t*, const size_t);
size_t GetGhostcells(MPI_File, uint8_t*, const size_t, uint8_t*, const size_t, const size_t, const size_t, size_t*, size_t*, const size_t);
void ReorganiseArray(uint8_t*, uint8_t*, uint8_t*, const size_t, const size_t, const size_t, const size_t*, size_t*);
uint8_t CollapseVoxels(uint8_t*, size_t*, const size_t);
size_t LowerLOD(MPI_File, uint8_t*, const size_t, size_t);
size_t GetNewLOD(MPI_File, MPI_File, size_t*, size_t*, size_t, const size_t, const size_t, size_t, int, int);

#endif
