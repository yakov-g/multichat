CC = gcc
CFLAGS = -Wall -Wextra -Wshadow -Wno-type-limits -g3 -O0 -Wpointer-arith -fvisibility=hidden

SOURCES = server.c bst_string.c
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE = server

#.o : .c
#	$(CC) $(CFLAGS) –c -o $@ $<

#%.o : %.c
#	$(CC) $(CFLAGS) –c -o $@ $<

all: $(EXECUTABLE)     

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -f *.o $(EXECUTABLE)
