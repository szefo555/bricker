#ifndef rw_h
#define rw_h

void Padding(FILE*, size_t);
void CopyLine(FILE*, FILE*, const size_t, const size_t, int*);
void CopyData(FILE*, FILE*, const size_t, const size_t, int*, size_t);
void Write(FILE*, size_t);


#endif
