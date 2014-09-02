#ifndef rw_h
#define rw_h

void Seek(MPI_File, int);
void Padding(MPI_File, size_t);
void CopyLine(MPI_File, MPI_File, int, size_t, int*);

#endif
