Test name:
    sample_test
 
Author(s):
    Saeed Mahameed
 
Short description:
    Sample Kernel level test that uses mlxkvl kernel testing infrastructure
 
Dependencies:
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Supported OSes:
    Linux
 
Run :
    make
    mlxkvl start
    insmod sample_test.ko
    mlxkvl dummy_test --help
    mlxkvl dummy_test dev=mlx4_0 port=2
    mlxkvl stop
 
Description:
    Dummy Test    
Known issues:
 
Todo:
    None
