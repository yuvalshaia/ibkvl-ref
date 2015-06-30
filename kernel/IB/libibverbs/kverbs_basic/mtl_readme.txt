Test name:
    kverbs_basic
    AKA : gen2_basic_kernel
 
Author(s):
    Saeed Mahameed
    Dotan Barak
 
Short description:
    Kernel level test for the core verbs API.
 
Dependencies:
    OFED
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Supported OSes:
    Linux
 
Run :
    make
    mlxkvl start
    insmod kverbs_basic.ko
    mlxkvl <testname> --help
    mlxkvl <testname> test=<test>

Description:
    The test is created to test the gen2 user level verbs against the IB spec 1.2.
    This means:
    1. Good flow of the verb:
        a. All legal parameters the verb can receive
        b. All legal combinations the verb can receive
    2. Bad flow of the verb:
        a. Out of range parameters
        b. Illegal combinations of legal and illegal parameters
        c. Things which are defined as bad flow in the IB spec
        d. Check that reference counters are being enforced

    The test is a one side test.

    Supported test cases:
        HCA:
                1. set and get client data
                2. register and unregister event handler
                3. dispatch event
                4. query port:
                        legal port number
                5. query GID:
                        legal values
                6. query PKEY:
                        legal values
                7. modify device with all of the attributes
                8. modify port with all of the attributes
        PD:
                1. alloc dealloc
                2. destroy with AV
                3. destroy with QP
                4. destroy with MR
                5. destroy with SRQ
        AV:
                1. create destroy:
                        create with legal parameters
                2. query AV
                3. modify AV
                4. create AV from WC

        CQ:
                1. create destroy:
                        create with random legal size
                        create with size o
                        cqe > HCA cap
                2. resize CQ
                       more than initial size
                        less than initial size
                        resize to invalid parameters
                3. destroy with QP
                4. modify with outstanding completions
                        more than number of outstanding
                        less than number of outstanding
                5. request notification and nnotification
       QP:
                1. create destroy:
                        legal random values
                        max WR + max SGE
                        RQ / SQ only
                2. modify QP in all possible transitions:
                        a. only required parameters
                        b. all optional parameters
                        c. illegal values for required parameters
                        d. illegal values for optional parameters
                        e. not all required parameters
                        f. not all optional parameters
                        g. invalid attributes
                   on good flow tests (a, b and f) also query QP
                3. Query init parameters
                4. Modify QP with APM parameters
        MULTICAST:
                1. attach detach:
                        attach detach with same legal GID & LID
                2. one QP to many multicast groups
                3. many QPs to one multicast group
        MR:
                1. reg dereg of dma MR:
                        reg with legal parameters
                2. alloc/dealloc/map/unmap FMR:
                        reg with legal parameters
                3. reg dereg phys MR:
                        reg with legal parameters
        SRQ:
                1. create destroy:
                        random legal size
                2. modify and query SRQ to check the SRQ limit
                3. try to destroy with QP
                4. create the SRQ limit event
        POLL POST
                1. ibv_post_send:
                        all sig types
                        all QP types
                        all legal opcodes
                        all legal send flags
                2. ibv_post_recv:
                        all sig types
                        all QP types
                        RTR & RTS
                3. ibv_post_srq_recv
                4. ibv_poll_cq:
                        max CQE, and poll one at a time
                        max CQE, and poll random number of completions at a time

Known issues:
    In case of test failure/kill the machine can stay in an unstable state.
    QP/SRQ are created with less than the maximum values (the kernel doesn't allow to create
    big resources because of lack of memory)
 
Todo:
    1. MW
    2. destroy PD with MW
    3. destroy QP with MW
    4. add support to change system image GUID
    5. add support to modify SQE->*
    6. create AH with static rate values different than 3
    7. add support to ib_req_ncomp_notif
    8. in modify QP: change the QP capabilities
    9. add support to query MR
    10.add SRQ resize support
    11.there are some more todo in the test
