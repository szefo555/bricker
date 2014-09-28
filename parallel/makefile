WARN = -Wall -Wextra -Werror -Wconversion
CC = mpicc
CFLAGS = $(WARN) -std=c99 -O3
LDFLAGS = -lm
HEADERS = lod.h brick.h send_settings.h misc.h
OBJECTS = main.o lod.o brick.o send_settings.o misc.o

all: bricker

%.o: %.c ($HEADERS)
	$(CC) -g $(CFLAGS) $WARN $< -o $(LDFLAGS)

bricker: $(OBJECTS)
	$(CC) -g $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o main b_* xmulti* stdout stderr


