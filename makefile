WARN = -Wall -Wextra -Werror -Wconversion
CFLAGS = -std=c99 -O3
LDFLAGS = -lm
OBJECTS = bricker.o

all: bricker

%.o: %.c ($HEADERS)
	$(CC) -g $(CFLAGS) $(WARN) $< -o $(LDFLAGS)

bricker: $(OBJECTS)
	$(CC) -g $(CFLAGS) $(OBJECTS) $(LDFLAGS) -o bricker

clean:
	rm -f *.o bricker stdout stderr
