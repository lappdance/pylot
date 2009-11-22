
all: ex_hello.sample ex_rw.sample ex_one_func.sample ex_array.sample ex_bundle.sample ex_collective.sample

%.sample: %.o
	mpicc $< -L../.. -lpilot -o $@

.c.o:
	mpicc $(CFLAGS) -I../.. -I$(CUNITHOME)/include -c $< -o $@

clean:
	$(RM) *.o *.sample
