WARN = -Wall -Wextra -Werror -Wconversion
CC = mpicc
CFLAGS = -std=c99 -O3
LDFLAGS = -lm
HEADERS = file_io.h rw.h
OBJECTS = bricker.o file_io.o rw.o

all: bricker

%.o: %.c ($HEADERS)
	$(CC) -g $(CFLAGS) $WARN $< -o $(LDFLAGS)

bricker: $(OBJECTS)
	$(CC) -g $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o bricker

clean:
	rm -f *.o bricker *.raw stdout stderr


