# Makefile for Pilot unit tests
# [02-Jun-09] Converted to link with libpilot.a (EM)
# [13-Aug-09] Clean deadlock tests' .job files (BG)

# make [all]	build regression tests suite (needs CUnit)
#		See 'run.sh' to run
# make dl	build deadlock tests
#		See 'deadlock_tests[_qsub].sh' to run

CC = mpicc

# narwhal.sharcnet.ca
# needs 'setenv LD_LIBRARY_PATH /work/wgardner/lib'
#CUNITHOME = /work/wgardner

# carmel, hermon
CUNITHOME = /usr/local

#CFLAGS = -Wall -g
CFLAGS =


all: libcheck test_suite

test_suite: unittests_main.o single_rw_suite.o array_rw_suite.o \
	mixed_value_suite.o selector_suite.o broadcaster_suite.o \
	gatherer_suite.o extra_read_write_suite.o format_suite.o \
	init_suite.o
	$(CC) $^ -L.. -lpilot -L$(CUNITHOME)/lib -lcunit -o test_suite

dl: deadlock/test_dead_wait.case \
	deadlock/test_dead_wait_select.case \
	deadlock/test_late_dead_wait_read.case \
	deadlock/test_late_dead_wait_write.case \
	deadlock/test_deadly_embrace.case \
	deadlock/test_unsatisfiable_select.case \
	deadlock/three_proc_cycle_read.case \
	deadlock/three_proc_cycle_write.case \
	deadlock/three_proc_cycle_select.case \
	deadlock/four_proc_cycle_read.case \
	deadlock/test_dead_wait_broadcast.case \
	deadlock/three_proc_cycle_gather.case

libcheck:
	@cd .. && $(MAKE)

clean:
	$(RM) *.o
	$(RM) test_suite
	$(RM) *.job* deadlock/*.case deadlock/*.o

%.case: %.o
	mpicc $< -L.. -lpilot -o $@

.c.o:
	mpicc $(CFLAGS) -I.. -I$(CUNITHOME)/include -c $< -o $@
