CC=gcc
HEADERS_GENERAL=../../include/
HEADERS_RPAL=../../include/rpal/
SOURCES=rpal_api.c rpal_array.c rpal_blob.c rpal_btree.c rpal_context.c rpal_debug.c rpal_endianness.c rpal_file.c rpal_getopt.c rpal_handleManager.c rpal_memory.c rpal_queue.c rpal_stack.c rpal_string.c rpal_stringbuffer.c rpal_synchronization.c rpal_threads.c rpal_time.c
OBJECTS=$(SOURCES:.c=.o)
CFLAGS=-c -Wall -I $(HEADERS_GENERAL) -I $(HEADERS_RPAL)
LDFLAGS=
EXECUTABLE=rpal
LDADD=-lrt

all: $(SOURCES) $(EXECUTABLE)

clean:
	rm -rf *.o
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LDADD)
	
.c.o:
	$(CC) $(CFLAGS) $< -o $@