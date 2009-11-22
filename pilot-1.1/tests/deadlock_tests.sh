#!/bin/bash
# This is the test runner for the deadlock unit tests. Since the
# deadlock detector kills the program when a deadlock is detected,
# there's no benefit to using CUnit as the test harness.

NPROCS=5	# no. of MPI/Pilot processes needed (becomes NSLOTS)

##################################################################
# Here are all the possible deadlock reasons in pilot_deadlock.c
#
REASON_DE="Conflicting channels create deadly embrace"
REASON_CW="Operation creates circular wait with above processes"
REASON_OX="Process at other end of channel has exited"
REASON_XH="Process exiting leaves earlier operation hung"
REASON_SN="Select cannot be fulfilled"
REASON_ES="Earlier select cannot be fulfilled"
##################################################################

find_text() {
    # $1 - text to search within
    # $2 - text to search for
    # $3 - alternate text to search for or ""
    [[ "$3" ]] && [[ "$1" =~ "$3" ]] || [[ "$1" =~ "$2" ]]
}

run_test() {
    # $1 - name of test executable
    # $2 - expected error code
    # $3 - optional alternate error code
    printf "  $1... "
    tmp=$(mpirun -np $NPROCS "deadlock/$1.case" -pisvc=d 2>&1 > /dev/null)
    if find_text "$tmp" "$2" "$3"; then
        echo "success"
    else
        echo "FAILURE"
        echo "$tmp"
    fi
}


starttime=`date`
echo "Running tests. Started $starttime..."

run_test "test_dead_wait"		"$REASON_OX" "$REASON_XH"
run_test "test_dead_wait_select"	"$REASON_SN" "$REASON_XH"
run_test "test_dead_wait_broadcast"	"$REASON_OX" "$REASON_XH"
run_test "test_late_dead_wait_read"	"$REASON_OX" "$REASON_XH"
run_test "test_late_dead_wait_write"	"$REASON_OX" "$REASON_XH"
run_test "test_deadly_embrace"		"$REASON_DE"
run_test "three_proc_cycle_read"	"$REASON_CW"
run_test "three_proc_cycle_write"	"$REASON_CW"
run_test "three_proc_cycle_select"	"$REASON_CW" "$REASON_SN"
run_test "three_proc_cycle_gather"	"$REASON_CW"
run_test "four_proc_cycle_read"		"$REASON_CW"
run_test "test_unsatisfiable_select"	"$REASON_SN" "$REASON_ES"

endtime=`date`
echo "Run ended on $endtime"
