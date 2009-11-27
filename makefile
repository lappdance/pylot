TARGET = pylot/_pylot.so

OBJECTS = ${addsuffix .o, \
	pylot \
	pylot_wrap \
	pilot-1.1/pilot \
	pilot-1.1/pilot_deadlock \
}

CC := mpicc
CFLAGS := -Wall -g -fPIC -c -I/usr/include/python2.5 -I./pilot-1.1
LDFLAGS := -shared

all : $(TARGET)

clean :
	-rm -f $(TARGET) $(OBJECTS) pylot_wrap.c pylot/pylot.py pylot/pylot.pyc

$(TARGET) : $(OBJECTS)
	$(CC) $(LDFLAGS) -o$@ $^

pylot_wrap.c pylot/pylot.py : pylot.i pylot.h
	swig -python -I./pilot-1.1 -outdir pylot $<

