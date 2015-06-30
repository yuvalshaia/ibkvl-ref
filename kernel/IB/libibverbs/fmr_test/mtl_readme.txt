Test name:
    fmr_test
 
Author(s):
    Dotan Barak
    Saeed Mahameed
 
Short description:
    Kernel level test that uses FMR (Fast Memory Regions).
 
Dependencies:
    OFED
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Supported OSes:
    Linux
 
Run :
    make
    mlxkvl start
    insmod fmr_ktest.ko
    mlxkvl fmr_test --help
 
Description:
    The test was written in order to check the FMRs in the kernel level.

    This test is a one sided test which has n pairs of threads.

    The test have a global data called "gdata" which all of the thread are using and 
    update the relevant data in it (every thread pair (and every thread in the pair) have
    a different place for it's data).

    Every pair send data from one thread to the other and they sync and exchange data
    using gdata attributes.

    The thread have 2 parts:
         1) User level agent that only connect to the kernel level part to tell him to 
            start working, and in the end get the status of the operation.

         2) Kernel level part which actually do the work. The main thread create all of the 
            thread pairs and wait for them to end.

    The kernel level part do the following scenario:
         * allocate the needed resources/memory
         * start the threads
         * wait for threads to end

    The working threads (the threads pairs) doing the following scenario:
         * create all of the needed IB resources
         * create many FMRs with random sizes and attributes (random number of pages, sizes)
         * Do in a loop:
              + connect the QPs 
              + map the FMR
              + post send request (with RDMA Write/Read) in the first thread in each pair
              + check the completion
              + validate the data
              + unmap the FMR
              + check that the rkey of the unmapped FMR is invalid

Known issues:
    In case of test failure/kill the machine can stay in an unstable state.
 
Todo:
    None
