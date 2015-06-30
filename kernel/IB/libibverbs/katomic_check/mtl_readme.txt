Test name:
    katomic_check
 
Author(s):
    Yohad Dikman
    Dotan Barak
    Saeed Mahameed
 
Short description:
    Kernel level test to check the atomic opcodes atomicity.
 
Dependencies:
    OFED
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Run :
    make
    mlxkvl start
    insmod atomic_test.ko
    katomic_test --help
     
Supported OSes:
    Linux
 

Description:
    The test was written in order to check that the atomic operations are keeping the
    atomicity of the operations in the kernel level.

    This test is a two sided test which connect through the user level.

    The thread have 2 parts:
         1) User level agent that connect to the kernel level part to tell him to 
            start working, exchange needed data between the user level in the other side
            in order to connect the QPs and in the end, get the status of the operation.

         2) Kernel level part which actually do the work.

    The kernel level part do the following scenario:
         * allocate the needed resources/memory
         * connect the QPs

    The following major flows are being checked:

        Fetch and Add testing:
             All the QPs tries to fetch and add to the same address, and in the end
             of the test, the manager check that all the ADDs were successfull.

        CMP & SWP testing:
             All of the QPs tries to compare and swap from the same address with the last known value
             (in the common address), and add a known value to this address,
             if it succeeded: then the new value with be set, otherwise the returned
             value will be used in the next post (this is the last "known value").
             At the end of the test, the master check for each side how many times he succeeded,
             and from this, it is known what should be the value in the shared address.

        For each flow, masked atomic operation (which is supported only by hermon) is being supported 
        too, and it can be used if the driver supports those feature.

Known issues:
    In case of test failure/kill the machine can stay in an unstable state.

    Default driver doesn't support masked atomic operations.
 
Todo:
    None
