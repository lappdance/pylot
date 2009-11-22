==========================
Pilot Programming Tutorial
==========================

:Author: John Douglas Carter
:Version: 1.1

.. contents::

Introduction
============

What is Pilot?
--------------

Pilot is a parallel programming library written in C, built on top of MPI.
Pilot provides an alternate development path for those wishing to write parallel
applications on an MPI cluster but feeling daunted by the MPI API.  Pilot
provides a higher level of abstraction that is based on Hoare's Communicating
Sequential Processes.  Pilot is designed with the novice parallel programmer in
mind, providing a range of safety mechanisms to steer developers away from
common MPI pitfalls.

For information about compiling and installing Pilot, see the `install guide`__.

__ http://carmel.cis.uoguelph.ca/pilot/docs/install/

Who is this guide for?
----------------------

This guide is designed as the starting point for all developers of Pilot
applications.  It presents all components of Pilot, including the two basic
concepts: Processes and Channels. In addition, this document provides an
overview of how to use Pilot's function calls.  We assume that potential
developers have the following skills:

* Basic C Programming: Familiarity with fprintf/fscanf, formatted I/O and
  functions.

      *Pilot does not actually rely on fprintf/fscanf, but it is designed to*
      *closely mirror those common C functions.*

* Knowledge of how to compile and run a program in an MPI cluster environment.

What Pilot isn't
----------------

* Pilot is not a programming language - it is a C library.

* Pilot is not MPI, but requires an MPI library be installed.

* Pilot is not tied to any specific interconnect (i.e., the way in which a
  cluster's computers pass messages).

* Pilot cannot automatically parallelize your program for you; you have to
  figure that out for yourself using the Pilot process/channel model.

* Pilot will not make your program run faster than an expert-coded
  implementation using purely MPI; but you needn't be an MPI expert to do
  cluster programming with Pilot!


Pilot Concepts
==============

Before we begin looking at the Pilot API, we need to introduce some essential
concepts that are borrowed from ways of thinking about concurrent computing such
as Hoare's Communicating Sequential Processes (CSP) and the Pi-calculus (which
gives Pilot its name). You do not need to understand CSP or the Pi-calculus,
just these two concepts:

Process
-------

For the purpose of cluster computing, we define a process as anything that is
capable of having behavior that can be executed on one of the nodes in an MPI
cluster. (If you know something about operating systems, this is the same idea
as an operating system process. If you about the distinction between processes
and threads, Pilot does not work with threads, only with processes.) The whole
purpose of cluster computing is to enable tens or hundreds of processes to
compute concurrently.

Channel
-------

A *channel* is a one-way communication medium between some pair of processes.

The following properties apply to channels:

* Channels have a single "write" end and a single "read" end. One process writes
  on the write end, and the other reads from the read end. It is like passing
  data through a pipe.

* A channel's ends are fixed to its two processes and cannot be changed to other
  processes.

* Channels are not dedicated to any particular type of data.

* When a channel is written to or read from, that process containing the
  read/write blocks (waits, stalls) until the operation is completed. That is,
  it is not necessary for the writer to write first, or the reader to read
  first. The data will pass when both processes are ready, after which both will
  continue on their separate ways. This is called synchronous communication. It
  is like using a telephone that has no ringer and no answering machine: The
  parties must have an arrangement to go pick up their respective phones, and
  only then can they talk with each other.

In terms of C programming, think about the type of a variable used to read and
write disk files. It has the type FILE* (pointer to a FILE
structure). Similarly, in Pilot we have PI_PROCESS* and PI_CHANNEL* types of
variables. Just as with multiple files, you can have arrays of PI_PROCESS* and
PI_CHANNEL*.

In a typical Pilot program, one master process will send data to a number of
parallel slave or worker processes, and then get results back. In such cases, it
can be very convenient to refer to the channels as a group, and so we have a
third variable type, the PI_BUNDLE*.

Bundle
------

A bundle is a grouping of channels with a shared endpoint for reading or
writing.  Bundles are used for operations that need to be applied to a group of
processes at the same time, such as writing the same value to all processes in
the group, reading a value from every process, or determining which channel in
the group is ready to be read. Strictly speaking, the use of bundles is
optional; you can get the same results using channels alone, but you will need
to write more code.

Applying the Concepts
---------------------

To design a Pilot application, one divides up the computation into a helpful
number of processes. That number can be determined ahead of time (say, if the
computation has three logical subdivisions) or can be adapted at run time
depending on the number of MPI nodes that are available in the cluster. When you
submit the program, you tell the system that you want, say, 120 processes, and
then it schedules your job when the resources are available. When it actually
begins to run, Pilot will let the program know that up to 120 processes can be
created (i.e., the main one plus 119 more). However, if the program is written
to insist on using 120 processes, then when fewer are available, Pilot will
print an error message and abort the program.

Unlike processes, whose quantity is constrained by the physical resources of
your cluster, there is no practical limit on the number of Pilot channels that
can be defined. They are purely a software abstraction and take up very few
computer resources.

Using processes and channels, you can structure your parallel computation into
typical architectures such as master/slave and pipeline patterns.


Anatomy of a Pilot Program
==========================

A pilot program contains the following:

* A "main" or master process (also referred to as PI_MAIN).

* A number of separate additional processes, each with an associated function
  that specifies its behavior.

* Channels connecting the additional processes to PI_MAIN and possibly to each
  other.

* Funnels providing channel groupings for collective operations.

* Symbols starting with ``PI_`` are defined in "pilot.h".

Example
-------

.. figure:: img/process_channel_diagram.png
   :alt: Process/channel diagram

   A simple 3-process program that consists of a main process, and two others, B
   and C, connected unidirectionally from PI_MAIN.  B and C have functions
   associated with them, as well as the subscript 2 and 3 respectively.

The code corresponding to this diagram is as follows::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  PI_CHANNEL *chan_B, *chan_C;
    5.  PI_PROCESS *B, *C;
    6.
    7.  int b( int q, void *p ) {
    8.      printf( "I AM Process B -- my subscript is %d\n", q );
    9.      return 0;
    10. }
    11.
    12. int c( int q, void *p ) {
    13.     printf( "I AM Process C -- my subscript is %d\n", q );
    14.     return 0;
    15. }
    16.
    17. int main( int argc,char *argv[] ) {
    18.     PI_Configure( &argc, &argv );
    19.
    20.     B = PI_CreateProcess( b, 2, NULL );
    21.     C = PI_CreateProcess( c, 3, NULL );
    22.     chan_B = PI_CreateChannel( PI_MAIN, B );
    23.     chan_C = PI_CreateChannel( PI_MAIN, C );
    24.
    25.     PI_StartAll();
    26.
    27.     printf( "I AM MAIN!\n" );
    28.
    29.     PI_StopMain( 0 );
    30.     return 0;
    31. }

Line 4 defines two variables for channels B and C.

Line 5 defines two variables for processes B and C.

Lines 7 - 15 define two functions which will be associated with processes B and
C, respectively. Note that the return value of these functions can be anything
that you want. The values are written to the log file to help you debug your
program.

Line 21: We create the ``B`` process and attack the function ``b`` to it. When
``b()`` is called to start its life as an independent process, the number 2 and
pointer ``NULL`` will be passed to it as arguments. The first argument is
intended for use as an index or subscript. The second can point to any arbitrary
data structure set up by ``main()``.

Line 22: As with process ``B`` (line 21), but substituting the values for
``C``. There is no reason why several processes cannot execute the same
associated function, say, in the typical pattern where many workers are
performing the same algorithm. E.g., both ``B`` and ``C`` could have been
specified to execute ``b()``. In this case, a worker process can conveniently
find out which instance it is via the integer or ``void*`` argument.

Lines 23–24: Creates channels from ``PI_MAIN`` to ``B`` and from ``PI_MAIN`` to
``C``.

Line 26: ``PI_StartAll()`` starts all earlier created processes on separate
processors of the cluster. ``B`` and ``C`` now execute their associated
functions ``b(2, NULL)`` and ``c(3, NULL)``, while the code in ``main()``
between ``PI_StartAll()`` and ``PI_StopMain(0)`` executing as the master process
running on its own processor.

Line 30: ``PI_StopMain()`` finalizes the underlying MPI library and does some
clean up. If logging is enabled, then the value you give ``PI_StopMain()`` will
appear in the log file.


Using Pilot - The Basics
========================

The smallest possible Pilot program::

    #include <pilot.h>

    int main( int argc, char *argv[] ) {

        /*
         * Initialize and configure the Pilot library.
         * Note that the address of argc and argv are required.
         */
        PI_Configure( &argc, &argv );

        /*
         * Start all created processes including the default
         * PI_MAIN process. In this case only PI_MAIN is started.
         */
        PI_StartAll();

        /*
         * Last command executed by Pilot. Causes all processes
         * to wait for completion before cleaning up.
         */
        PI_StopMain( 0 );

        return 0;
    }


Channel Input and Output
========================

You may have noticed in the previous example that we created two channels but
did not do anything with them.  The channels created are from PI_MAIN to process
B and C.  Now we will modify the program so that each process receives two
integers from PI_MAIN and outputs the sum.

The channel read function looks like::

    PI_Read( [ channel variable ] , [ format string ], [ list of variable addresses ] );

* The first parameter is the channel pointer created using PI_CreateChannel.

* The second parameter, “format string” is a character string containing a list of
  fscanf-style format codes for the values to read from the channel. The
  most common codes are:

  * %d or %i - Used for integers (int)
  * %c - Used for characters (char)
  * %f - Used for floating points (float)
  * %lf - Used for double precision floating points (double)

  Since Version 1.1 Pilot has additional formats:

  * %hhu - unsigned char
  * %hd - short int
  * %ld	- long int
  * %lld -long long int
  * %u - unsigned int
  * %Lf - long double

  "u" can be used to specify other unsigned integer types:  hu, lu, llu.
  There are also two "advanced" types:

  * %b - Used to send uninterpreted 8-bit bytes without any conversion (e.g, for endian, character code, or sign convention differences); corresponds to MPI_BYTE
  * %m - Used for user-defined types, such as structs; requires calling MPI functions to set up the type in a MPI_Datatype variable

* The third parameter is not a single parameter per se, but rather a
  comma-separated list of variable addresses that will be used to store the
  values read from the channel.  As with fscanf(), each variable requires the
  '&'. Just as with fscanf(), forgetting '&' will likely cause a program fault,
  because the variable's value will be treated as an address.

The channel write function looks like::

    PI_Write( [ channel variable ], [ format string ], [ list of values ] );

The syntax of PI_Write is identical to that PI_Read, and very similar to
printf() usage.

So far, we are only showing how to write/read individual (i.e., “scalar”) data
values. Multiple scalars can be transferred in one PI_Write/Read, just by
putting additional codes in the format string and listing more arguments. The
method of communicating arrays (shown later) needs only a slight
modification. In all cases, Pilot treats any PI_Read or PI_Write function as a
single communication event regardless how many scalars (or arrays) are involved.

Let us examine the program after modifying it to use PI_Read and PI_Write::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  PI_CHANNEL *chan_B, *chan_C;
    5.  PI_PROCESS *B, *C;
    6.
    7.  int b( int q, void *p ) {
    8.      int x, y, sum;
    9.
    10.     PI_Read( chan_B, "%d%d", &x, &y );
    11.     sum = x + y;
    12.     printf( "The sum of %d and %d is %d\n", x, y, sum );
    13.
    14.     return 0;
    15. }
    16.
    17. int c( int q, void *p ) {
    18.     float x, y, sum;
    19.
    20.     PI_Read( chan_C, "%f%f", &x, &y );
    21.     sum = x + y;
    22.     printf( "The sum of %f and %f is %f\n", x, y, sum );
    23.
    24.     return 0;
    25. }
    26.
    27. int main( int argc, char *argv[] ) {
    28.     PI_Configure( &argc, &argv );
    29.
    30.     B = PI_CreateProcess( b, 2, NULL );
    31.     C = PI_CreateProcess( c, 3, NULL );
    32.     chan_B = PI_CreateChannel( PI_MAIN, B );
    33.     chan_C = PI_CreateChannel( PI_MAIN, C );
    34.
    35.     PI_StartAll();
    36.
    37.     PI_Write( chan_B, "%d%d", 50, 25 );
    38.     PI_Write( chan_C, "%f%f", 2.501f, 10.00f );
    39.
    40.     PI_StopMain( 0 );
    41.
    42.     return 0;
    43. }

Above, we have replaced the function bodies of b and c with code that reads from
main.  B receives two ints, C receives floats.  Both processes sum the two
values they've read, and output this sum. (Note that on some clusters, only the
main process may be allowed to print to stdout.)

Important Points:

1. Line 10 and Line 20: Notice that the scalar variables are prefixed with &.
   PI_Read is just like fscanf in this requirement.

2. Lines 38 and 39: Here are the writes corresponding to the reads previously
   discussed. Here, the values are both literals.

More About Processes
====================

Every process has a function associated with it.  When a process is created by
PI_CreateProcess, it is assigned a function that contains the behavior for that
process.  Multiple processes can utilize the same function definition, since
separate copies will be executing concurrently on different processors of the
cluster. (These processes have their own copies of all variables, too. The only
way to share data between processes is explicitly via channels.)

    *Warning: On a cluster that features shared memory processors, non-stack*
    *variables may indeed be shared among several processes. This is a risky*
    *situation that the novice programmer should avoid by sticking to channel*
    *communication.*

The prototype for a process function is::

    int work_func( int q, void *p );

The arguments of these functions are as follows:

* int q - An integer argument passed to the function when it is invoked.
  Typically this is used as a “subscript” for processes.  For instance, if you
  wanted to use the same function for a number of processes, but needed a way to
  configure each process, the subscript provides a convenient way to do it.

* void \*p - A void pointer that can be used to pass any additional data into the
  process function.

Now we will modify our previous example to use the same function for all
processes, making use of the subscript.

The code listing below shows the changes::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  PI_CHANNEL *chans[2];
    5.  PI_PROCESS *procs[2];
    6.
    7.  int WorkFunc( int q, void *p ) {
    8.      int x, y, sum;
    9.
    10.     PI_Read( chans[q], "%d%d", &x, &y );
    11.     sum = x + y;
    12.     printf( "The sum of %d and %d is %d\n", x, y, sum );
    13.
    14.     return 0;
    15. }
    16.
    17. int main( int argc, char *argv[] ) {
    18.     int i;
    19.
    20.     PI_Configure( &argc, &argv );
    21.
    22.     for ( i = 0; i < 2; i++ ) {
    23.         procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
    24.         chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
    25.     }
    26.
    27.     PI_StartAll();
    28.
    29.     PI_Write( chans[0], "%d%d", 50, 25 );
    30.     PI_Write( chans[1], "%d%d", -100, 25 );
    31.
    32.     PI_StopMain( 0 );
    33.     return 0;
    34. }

Lines 4-5: Note that we have changed the ``PI_CHANNEL`` and ``PI_PROCESS``
pointers to arrays of size 2.  The process subscript will be used to access the
appropriate channel from the process function.

Line 7-15: We've modified the function to operate only on *ints*, and
generalized it using process subscripts -- it now reads from the channel at
position ``q`` in the channel array.

Line 29-30: ``PI_MAIN`` now writes to indexed channels.

Using process subscripts results in shorter, more compact programs and more
general code.  When designing your algorithms, it is wise to think about how the
algorithm may be generalized for N processes. You can fix N in your program when
you design it, or you can wait until run time and let N vary with the number of
processes available. That number is available as the return value of
``PI_Configure()``.

A brief word should be inserted about ``PI_StartAll()``. Between
``PI_Configure()`` and ``PI_StartAll()``, your program is in a “setup” phase
where you are creating processes and channels. The same setup code is being
executed concurrently on all processors, so that every process will have the
same process and channel definitions. Upon ``PI_StartAll()``, all the processes
go their separate ways, with the master process continuing as ``main()``. At
that point, defining more processes and channels is no longer permitted. Pilot
will stop with an error if you do that.

Next, we will modify our program to use arrays instead single ints.


Channels and Arrays
===================

In addition to single types (int, char, float, double) PI_Read and PI_Write also
allows for arrays to be written to and read from channels.  This is accomplished
using slightly modified format codes that are similar to fprintf/fscanf's
convention for specifying field width:

* %nd
* %nc
* %nf
* %nlf

Each of the above reads or writes an array of length ``n`` of the specified
type.  For instance, to read an integer array of length 50, the PI_Read would
look like::

    PI_Read( aChannel, "%50d", arrayname );

Since arrayname is an array, it resolves to the address of its first element,
hence there is no need for '&'.

Programmatically Specifying Array Size
--------------------------------------

We do not always know the size of an array at compile time, so Pilot provides a
way to specify it using a variable.

When an asterisk (*) is substituted into the format code for ``n``, an integer
argument containing the size of the array is expected to be inserted into the
variable list before the array variable.  For instance, rewriting the previous
PI_Read gives us the equivalent communication::

    PI_Read( aChannel, "%*d", 50, arrayname );

Next we will rewrite our running example to send integer arrays to each of the
processes and write back the sum of all integers in the array::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  PI_CHANNEL *chans[2];
    5.  PI_CHANNEL *chans_out[2];
    6.  PI_PROCESS *procs[2];
    7.
    8.  int WorkFunc( int q, void *p ) {
    9.      int i, sum;
    10.     int array[10];
    11.
    12.     PI_Read( chans[q], "%10d", array );
    13.
    14.     sum = 0;
    15.     for ( i = 0; i < 10; i++ ) {
    16.         sum += array[i];
    17.     }
    18.
    19.     PI_Write( chans_out[q], "%d", sum );
    20.
    21.     return 0;
    22. }
    23.
    24. int main( int argc, char *argv[] ) {
    25.     int i;
    26.     int array_a[10];
    27.     int array_b[10];
    28.     int sum_a, sum_b;
    29.
    30.     for ( i = 0; i < 10; i++ ) {
    31.         array_a[i] = i + 2;
    32.         array_b[i] = i + 3;
    33.     }
    34.
    35.     PI_Configure( &argc, &argv );
    36.
    37.     for ( i = 0; i < 2; i++ ) {
    38.         procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
    39.         chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
    40.         chans_out[i] = PI_CreateChannel( procs[i], PI_MAIN );
    41.     }
    42.
    43.     PI_StartAll();
    44.
    45.     PI_Write( chans[0], "%10d", array_a );
    46.     PI_Write( chans[1], "%10d", array_b );
    47.
    48.     PI_Read( chans_out[0], "%d", &sum_a );
    49.     PI_Read( chans_out[1], "%d", &sum_b );
    50.
    51.     printf( "\nThe sum of A is: %d.\nThe sum of B is: %d.\n", sum_a, sum_b );
    52.
    53.     PI_StopMain( 0 );
    54.     return 0;
    55. }

Note lines 45 - 49: The read operation is blocking so it is smarter to perform
all your write operations before any of your read operations.

Exercise
--------

Try modifying the program to do the following:

* Introduce a variable to represent the size of the arrays ``a`` and ``b``. This
  can be read from stdin or as a command line argument.

* Initialize arrays ``a`` and ``b`` accordingly.

* Before sending the arrays, send the user-inputted array size to each of the
  two processes.

* Have the ``WorkFunc`` function programmatically specify the array size (just
  read from main) in its statements for reading the array.


Bundles and Selection
=====================

In looking at lines 48-49 in the latest iteration of our running example, we see
that we are reading the results from each process in order, starting with 0 and
ending with 1.  If process A has not yet sent the result, we will wait, receive,
and then proceed to receiving the result from process B.  But if B's result had
been ready first, forcing the sequence of “A first, then B” would result in a
small performance impact: Main would be sitting idle waiting for a result when
it could be getting the result from a process that has already finished.

This problem is trivial with only two processes; however, imagine the
performance penalty of having 1,000 processes performing work and receiving the
data from them in a preprogrammed order.  To speed up processing in this common
situation, what is desired is a mechanism to simultaneously “wait” on a number
of channels, and wake up when any one is ready to communicate data. This
mechanism requires two ingredients: (1) a means of collecting channels into a
group; and (2) a function that waits on the group as a whole. In Pilot, the
collection of channels is called a “bundle” (from the fact that the channels
share a common endpoint), and the function is PI_Select.

Later, we will encounter more kinds of bundles in Pilot. The kind that is used
with PI_SELECT is known as a Selector.

Creating Selectors
------------------

::

  PI_BUNDLE* PI_CreatBundle( PI_SELECT, [Channels], [Number of Channels] );

This creates a selector bundle. The arguments are as follows:

* Channels. An array of channels to build the selector from.  All of the
  channels contained in this array should have the same read end, and distinct
  write ends. The reading process may be PI_MAIN, but it does not have to be.

* Number of Channels. The number of channels in the array.

PI_CreateBundle, using PI_SELECT as the first argument, returns a pointer to the
newly created selector bundle.

Selecting the Channel to Read
-----------------------------

After creating a selector bundle, and after a number of processes have written
to it, we need to test to see which channel is ready to read from. This is
accomplished using the PI_Select function::

    int PI_Select( PI_BUNDLE* bundle );

PI_Select takes a selector bundle as its sole argument and returns the integer
index of a channel that is ready to be read. The index will be the same as that
of the channel in the original PI_CreateSelector call.

In case that array is no longer available, the channel can be also recovered
directly from the bundle using this function::

    PI_CHANNEL* PI_GetBundleChannel( const PI_BUNDLE* bundle, int index );

PI_GetBundleChannel returns the channel pointer at the specified index. If the
program now calls PI_Read on that channel, the read will immediately
succeed. Note that PI_Select will not return until some channel in the bundle is
ready to read. It is up to the reading process not to call PI_Select too many
times.

Next we have modified the running example to use PI_Select to read from process
A and B as the data becomes available (instead of in arbitrary order)::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  PI_CHANNEL *chans[2];
    5.  PI_CHANNEL *chans_out[2];
    6.  PI_PROCESS *procs[2];
    7.  PI_BUNDLE *bundle;
    8.
    9.  int WorkFunc( int q, void *p ) {
    10.     int i, sum;
    11.     int array[10];
    12.
    13.     PI_Read( chans[q], "%10d", array );
    14.
    15.     sum = 0;
    16.     for ( i = 0; i < 10; i++ ) {
    17.         sum += array[i];
    18.     }
    19.
    20.     PI_Write( chans_out[q], "%d", sum );
    21.
    22.     return 0;
    23. }
    24.
    25. int main( int argc, char *argv[] ) {
    26.     int i;
    27.     int array_a[10];
    28.     int array_b[10];
    29.     int sum;
    30.
    31.     for ( i = 0; i < 10; i++ ) {
    32.         array_a[i] = i + 2;
    33.         array_b[i] = i + 3;
    34.     }
    35.
    36.     PI_Configure( &argc, &argv );
    37.
    38.     for ( i = 0; i < 2; i++ ) {
    39.         procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
    40.         chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
    41.         chans_out[i] = PI_CreateChannel( procs[i], PI_MAIN );
    42.     }
    43.     bundle = PI_CreateBundle( PI_SELECT, chans_out, 2 );
    44.
    45.     PI_StartAll();
    46.
    47.     PI_Write( chans[0], "%10d", array_a );
    48.     PI_Write( chans[1], "%10d", array_b );
    49.
    50.     for ( i = 0; i < 2; i++ ) {
    51.         int selected = PI_Select( bundle );
    52.         PI_Read( PI_GetBundleChannel( bundle, selected ), "%d", &sum );
    53.         printf( "Process %d is ready -- The sum is %d.\n", selected, sum );
    54.     }
    55.
    56.     PI_StopMain( 0 );
    57.     return 0;
    58. }

Line 7: We've added a pointer to a bundle.

Line 9 - 23: Note that ``WorkFunc()`` has not been modified.

Line 43: Here we create the selector bundle, passing our array of output
channels.

Lines 50 - 54: We have created a for loop, which is used to execute two
PI_Select statements.

Line 51: PI_Select is called, and the index of a channel that is ready to stored
in selected.

Line 52: Here, we are reading from the selected channel with a nested call to
PI_GetBundleChannel() inside PI_Read.

When this program is executed multiple times on a cluster, the program output
may vary depending on which process writes to its output channel first.

The output might be::

    Process 0 is ready -- The sum is 65.
    Process 1 is ready -- The sum is 75.

or sometimes::

    Process 1 is ready -- The sum is 75.
    Process 0 is ready -- The sum is 65.


Collective Operations
=====================

In our running example, we have been reading and writing from processes
explicitly. But in typical parallel programs, where many worker processes are
computing concurrently, it is useful to have more efficient means of farming out
data to the workers and collecting results. These functions are optional; they
do not do anything you cannot do using channels alone by writing more code.

Pilot (like the underlying MPI) provides operations that write to or read from a
group of processes at once.  As we did for channel selection, we again use
bundles to designate groups of channels for collective treatment. The additional
bundle types are Broadcaster and Gatherer:

Broadcasters are used to write the same data to a group of processes at one
time. Gatherers are used to read data from a group of processes at one time.

Bundles for Broadcasters and Gatherers are created in the same way as Selectors,
and take the same arguments::

    PI_BUNDLE* PI_CreateBundle( PI_BROADCAST, [Channels], [Number of Channels] )
    PI_BUNDLE* PI_CreateBundle( PI_GATHER, [Channels], [Number of Channels] )

Broadcasting
------------

Writing to a Broadcaster bundle is accomplished using ``PI_Broadcast``, whose
arguments are the same as for ``PI_Write``::

    void PI_Broadcast([Broadcaster bundle], [format string], [list of arrays])

At the “read” end of a Broadcast, the individual processes simply call
``PI_Read`` as usual. Every channel in the bundle will carry the same data to
its respective processes.

Gathering
---------

Reading from a Gatherer bundle is accomplished using ``PI_Gather``, whose
arguments are almost the same as for ``PI_Read``::

    void PI_Gather([Gatherer bundle], [format string] [list of arrays])

The difference from a conventional ``PI_Read`` is as follows:

* Each format code specifies the data that each process is sending (i.e., it
  matches the PI_Write format string).

* But, each address argument must be an array capable of holding the data from
  all n channels in the bundle.

That is, for ``PI_Read`` a scalar format code is paired with a scalar variable
address, while an array format code is paired with an array address. But for
``PI_Gather``, every read results in filling an n-element array with
contributions from the n channels (in the same order as the original
``PI_CreateBundle`` call). Thus, a scalar format code is paired with an array
address, and an array format code is also paired with an array address (whose
storage must accommodate n times the length of the array format).

For instance, if the format code is ``"%5d"`` (5 integers) and we are gathering
from 4 processes, the array used for storage of the ``"%5d"`` must be at an
integer array of at least size 20 (5 * 4); e.g.::

    int anArray[20];
    PI_Gather( aBundle, "%5d", anArray );

At the “write” end of a gather, the individual processes simply call
``PI_Write`` as usual.

Broadcaster and Gatherer Example
--------------------------------

To illustrate the usage of PI_Gather and PI_Broadcast, we'll modify our running
example according to the following specification:

* Send (broadcast) the same array of 5 integers to all processes.

* Each process then adds its subscript to all integers in the broadcasted array.

* The main process gathers all the modified arrays from each process, and then
  calculates and outputs the average.

These changes have been made in the code listing below::

    1.  #include <stdio.h>
    2.  #include <pilot.h>
    3.
    4.  #define NUM_PROCS  2
    5.  #define NUM_CHANS  2
    6.
    7.  PI_CHANNEL *chans[ NUM_CHANS ];
    8.  PI_CHANNEL *chans_out[ NUM_CHANS ];
    9.  PI_PROCESS *procs[ NUM_PROCS ];
    10. PI_BUNDLE *gather_bundle;
    11. PI_BUNDLE *broadcast_bundle;
    12.
    13. int WorkFunc( int q, void *p ) {
    14.     int i;
    15.     int array[5];
    16.
    17.     PI_Read( chans[q], "%5d", array );
    18.
    19.     for ( i = 0; i < 5; i++ ) {
    20.         array[i] = array[i] + q;
    21.     }
    22.
    23.     PI_Write( chans_out[q], "%5d", array );
    24.     return 0;
    25. }
    26.
    27. int main( int argc, char *argv[] ) {
    28.     int i, sum;
    29.     int send[5];
    30.     int recv[5 * NUM_CHANS];
    31.     float avg;
    32.
    33.     for ( i = 0; i < 5; i++ ) {
    34.         send[i] = i + 1;
    35.     }
    36.
    37.     PI_Configure( &argc, &argv );
    38.
    39.     for ( i = 0; i < NUM_PROCS; i++ ) {
    40.         procs[i] = PI_CreateProcess( WorkFunc, i, NULL );
    41.         chans[i] = PI_CreateChannel( PI_MAIN, procs[i] );
    42.         chans_out[i] = PI_CreateChannel( procs[i], PI_MAIN );
    43.     }
    44.     broadcast_bundle = PI_CreateBundle( PI_BROADCAST, chans, NUM_CHANS );
    45.     gather_bundle = PI_CreateBundle( PI_GATHER, chans_out, NUM_CHANS );
    46.
    47.     PI_StartAll();
    48.
    49.     PI_Broadcast( broadcast_bundle, "%5d", send );
    50.     PI_Gather( gather_bundle, "%5d", recv );
    51.
    52.     sum = 0;
    53.     for ( i = 0; i < 5 * NUM_CHANS; i++ ) {
    54.         sum += recv[i];
    55.     }
    56.     avg = sum / ( 5.0f * NUM_CHANS );
    57.
    58.     printf( "\nThe average was: %f\n", avg );
    59.
    60.     PI_StopMain( 0 );
    61.     return 0;
    62. }

Lines 10, 11: We define two bundle variables -- one for the broadcaster, one for
the gatherer.

Lines 13 - 25: Note that the ``WorkFunc`` uses only ``PI_Read`` and ``PI_Write`` -
this end of the bundle does not need to code anything special in order to
participate in broadcast and/or gather.

Lines 44 - 45: We create the broadcaster and gatherer bundles, using the
previously created channel arrays.

Line 49: We call PI_Broadcast, sending the integer array (of size 5) to all
process on the reading end of the bundle.

Line 50: We call PI_Gather, which reads an integer array (of size 5) from each
process on the writing end of the bundle. Note that this is read into an array
of size 10, since two processes are each writing an array of size 5 to the
bundle.


Conclusion
==========

Although it may seem as though we have only introduced a very small number of
function calls, we have covered the usage of 90% of the Pilot API.  The beauty
of Pilot is its simplicity, and familiarity for programmers comfortable with C.

It is our hope that after reading this guide, a HPC developer would be able to
use Pilot and apply the concept of Processes and Channels to the design of
parallel algorithms.

..
    Local Variables:
    mode: rst
    fill-column: 80
    End:
