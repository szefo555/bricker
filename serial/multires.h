#ifndef multires_h
#define multires_h

size_t ReadAndAdd(FILE*, const size_t, size_t);
void CalcMultiresolutionHierarchy(FILE*, FILE*, size_t, const size_t, const size_t, const size_t, bool, size_t, size_t*); 

#endif
