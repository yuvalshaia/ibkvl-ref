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

USER_APP_NAME = "./katomic_test"

class KAtomicTest(MlxKvlTest):
 
    def setup (self):
        return self.kvl_setup()
    
    def run (self):
        case_args = " ".join(self.args)
        test_args = "--device=%s --ib_port=%s --seed=%s" % (self.mydev,str(self.port),str(self.SEED))
        if self.is_server:
            test_args += " --daemon "
        else:
            test_args += " --ip=%s" % (self.partnerIP)
        rc = self.run_user_app(user_app = USER_APP_NAME,args = test_args+" " +case_args)
        status = Status.SUCCESS
        if rc:
            status = Status.FAIL
        return status
 
    def clean_up (self):
        return self.kvl_cleanup()
 
if __name__ == "__main__":
    test = KAtomicTest("atomic_test",info = "Kernel IB atomic Operations Test")
    rc = test.execute()
    sys.exit(rc)
