WARN = -Wall -Wextra -Werror -Wconversion
CFLAGS = -std=c99 -O3
LDFLAGS = -lm
HEADERS = file_io.h rw.h multires.h
OBJECTS = rw.o file_io.o bricker.o multires.o

all: bricker

%.o: %.c ($HEADERS)
	$(CC) -g $(CFLAGS) $(WARN) $< -o $(LDFLAGS)

bricker: $(OBJECTS)
	$(CC) -g $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o bricker

clean:
	rm -f *.o bricker stdout stderr *.raw

