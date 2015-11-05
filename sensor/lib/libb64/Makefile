BINARIES = decoder encoder b64dec b64enc
LIBRARIES = libb64.a

SOURCES = b64dec.c  b64enc.c  cdecode.c  cencode.c  decoder.cc  encoder.cc

TARGETS = $(LIBRARIES) $(BINARIES)

LINK.o = g++

CFLAGS:= -I../include
CXXFLAGS:= -I../include

vpath %.h ../include/b64

.PHONY : clean

all: $(TARGETS) #strip

b64enc b64dec encoder decoder: libb64.a
libb64.a: cencode.o cdecode.o
	$(AR) $(ARFLAGS) $@ $^

strip:
	strip $(BINARIES)

clean:
	rm -f *.exe* *.o $(TARGETS) *.bak *~

distclean: clean
	rm -f depend

depend: $(SOURCES)
	makedepend -f- $(CFLAGS) $(SOURCES) 2> /dev/null 1> depend

-include depend

