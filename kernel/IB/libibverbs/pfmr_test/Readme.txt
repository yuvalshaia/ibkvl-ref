Test name:
   	pfmr_test 
 
Author(s):
    Saeed Mahameed
 
Short description:
    kernel test to test protected FMRs
 
Dependencies:
    MLXKVL (/mswg/projects/ver_tools/latest/mlxkvl_install.sh)
 
Supported OSes:
    Linux
 
Run :
    make
    mlxkvl start
    insmod pfmr_test.ko
    mlxkvl pfmr_test --help
    mlxkvl pfmr_test dev=mlx4_0 port=2
    mlxkvl stop
 
Description:
   	N/A

Known issues:
	N/A
 
Todo:
	N/A

