#ifndef multires_h
#define multires_h

size_t ReadAndAdd(MPI_File, const size_t, size_t);
void CalcMultiresolutionHierarchy(MPI_File, MPI_File, size_t, const size_t, const size_t, const size_t, bool, size_t, size_t*, int); 

#endif
