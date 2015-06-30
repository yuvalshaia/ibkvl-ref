#!/usr/bin/env python
# -*- python -*-
# $Id:      katmoic_runner.py
# Author:  Saeed Mahameed   saeedm@mellanox.com --  Created: 30.12.11

__author__ =  'Saeed Mahameed - saeedm@mellanox.co.il'
__version__=  '1.0'

import os
import sys

from mlxlib.common import trace,execute,mlx_platform,files
from mlxlib.regression.MlxKvlTest import MlxKvlTest
from mlxlib.regression.ATest import Status

KVL_OP_NAME = "fmr_test"
class FMRTest(MlxKvlTest):
 
    def setup (self):
        return self.kvl_setup()
    
    def run (self):
        case_args = " ".join(self.args)
        test_args = "dev=%s ibport=%s seed=%s" % (self.mydev,str(self.port),str(self.SEED))
        rc = self.run_mlxkvl(kvl_op = KVL_OP_NAME,args = test_args+" " +case_args)
        status = Status.SUCCESS
        if rc:
            status = Status.FAIL
        return status
 
    def clean_up (self):
        return self.kvl_cleanup()
 
 
if __name__ == "__main__":
    test = FMRTest("fmr_ktest",info = "Fast memory region test")
    rc = test.execute()
    sys.exit(rc)
