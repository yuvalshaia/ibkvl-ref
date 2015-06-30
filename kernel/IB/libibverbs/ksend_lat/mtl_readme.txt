Test name:
    ksend_lat
 
Author(s):
    Liran Liss 
    Dotan Barak
    Saeed Mahameed
 
Short description:
    Kernel level test benchmark of send latency.
 
Dependencies:
    OFED
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Run :
    make
    mlxkvl start
    insmod ksend_lat_mod.ko
    ib_send_lat --help

Supported OSes:
    Linux
 
Description:
    This test executes a simple ping-pong test in the kernel between two nodes, 
    using send-receive semantics.

    Receive detection is achieved by polling the receive queue (and not the data). 
    Three types of receive contexts are supported:
         (1) polling in a process context
         (2) blocking receive - if the receive CQ is empty, the test process requests a completion 
             notification and sleeps on a semaphore. When response message is received, the notification 
             handler signals the semaphore.
         (3) interrupt - apart from the initial 'ping' sent by the client node, the whole test 
             is conducted in interrupt context, i.e., responses are posted from within the 
             completion notification handler.

    The thread have 2 parts:
         1) User level agent that connect to the kernel level part to tell him to 
            start working, exchange needed data between the user level in the other side
            in order to connect the QPs and in the end, get the status of the operation.

         2) Kernel level part which actually do the work.

Known issues:
    In case of test failure/kill the machine can stay in an unstable state.

Todo:
    1) Add support for UC transport
    2) Increase the supported payload sizes to the order of several MBs by allocating with 
       vmalloc instead of kmalloc.
    3) Add an option to determine when to use inline sends. Currently, the test tries to
       inline any payload size smaller than 400 bytes.
       Note that the current kernel HCA driver ignores inlining anyway...
