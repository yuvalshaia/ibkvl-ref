#!/usr/bin/env python
# -*- python -*-
# $Id:      fmr_test_runner.py
# Author:  Saeed Mahameed   saeedm@mellanox.com --  Created: 30.12.11
 
__author__ =  'Saeed Mahameed - saeedm@mellanox.co.il'
__version__=  '1.0'
 
import os
import sys
 
from mlxlib.common import trace,execute,mlx_platform,files
from mlxlib.regression.MlxKvlTest import MlxKvlTest
from mlxlib.regression.ATest import Status

USER_APP_NAME = "./ib_ksend_lat"

class KSendLatTest(MlxKvlTest):
 
    def setup (self):
        return self.kvl_setup()
    
    def run (self):
        case_args = " ".join(self.args)
        test_args = "--ib-dev=%s --ib_port=%s" % (self.mydev,str(self.port))
        if not self.is_server:
            test_args += " --ip=%s" % (self.partnerIP)
        rc = self.run_user_app(user_app = USER_APP_NAME,args = test_args+" " +case_args)
        status = Status.SUCCESS
        if rc:
            status = Status.FAIL
        return status
 
    def clean_up (self):
        return self.kvl_cleanup()
 
 
if __name__ == "__main__":
    test = KSendLatTest("ksend_lat_mod",info = "Fast memory region test")
    rc = test.execute()
    sys.exit(rc)
