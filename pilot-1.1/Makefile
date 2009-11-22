# Makefile for Pilot library
#
# make [all]	build library
# make install	copy library to $PREFIX/lib and user header files to
#		$PREFIX/include, creating those directories if they do not
#		already exist.
# make install-man  create docs subdir, generate doxygen HTML and man pages,
#		then copy man pages to $DOC_PREFIX/man, creating if not exist

PREFIX = /usr/local
DOC_PREFIX = /usr/local

# Compiling:
#  on HP mpicc, gets lots of "type-punning" warnings w/o -fno-strict-aliasing
#  non-gcc may not like -Wall
CFLAGS = -Wall -fno-strict-aliasing -fPIC
CC = mpicc
#CFLAGS =

all: ../libpilot.so

../libpilot.so: pilot.o pilot_deadlock.o
	$(CC) -shared -o$@ pilot.o pilot_deadlock.o

pilot_private.h: pilot_limits.h

pilot_deadlock.h: pilot.h pilot_private.h

pilot.o: pilot.c pilot.h pilot_error.h pilot_private.h pilot_deadlock.h
	$(CC) $(CFLAGS) -c pilot.c -o pilot.o

pilot_deadlock.o: pilot_deadlock.c pilot_deadlock.h
	$(CC) $(CFLAGS) -c pilot_deadlock.c -o pilot_deadlock.o

install: libpilot.a
	mkdir -p $(PREFIX)/include/ && \
	cp pilot.h pilot_limits.h $(PREFIX)/include/ && \
	mkdir -p $(PREFIX)/lib/ && \
	cp libpilot.a $(PREFIX)/lib/

install-man:
	cd docs/ && $(MAKE) && cd .. && \
	mkdir -p $(DOC_PREFIX) && \
	cp -r docs/man $(DOC_PREFIX)

clean:
	rm -f *.a
	rm -f *.o
