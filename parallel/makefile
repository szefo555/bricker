WARN = -Wall -Wextra -Werror -Wconversion
CC = mpicc
CFLAGS = -std=c99 -O3
LDFLAGS = -lm
HEADERS = lod.h brick.h mpi_init.h misc.h
OBJECTS = main.o lod.o brick.o mpi_init.o misc.o

all: bricker

%.o: %.c ($HEADERS)
	$(CC) -g $(CFLAGS) $WARN $< -o $(LDFLAGS)

bricker: $(OBJECTS)
	$(CC) -g $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o main *.raw stdout stderr


