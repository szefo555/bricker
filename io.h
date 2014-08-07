#ifndef io_h
#define io_h

size_t read_data(const char* , const size_t*, uint8_t*);
size_t write_data(const size_t, size_t, uint8_t*);
size_t OpenFile(FILE**, size_t);

#endif
