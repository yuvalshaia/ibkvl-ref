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

MODULE_NAME = "kverbs_basic"

class KVerbsBasicTest(MlxKvlTest):
 
    def setup (self):
        return self.kvl_setup("device=%s" % self.mydev)
    
    def run (self):
        case_args = " ".join(self.args)
        rc = self.run_mlxkvl(kvl_op = "",args = case_args) # kvl_op is in case args
        status = Status.SUCCESS
        if rc:
            status = Status.FAIL
        return status
 
    def clean_up (self):
        return self.kvl_cleanup()
 
 
if __name__ == "__main__":
    test = KVerbsBasicTest(MODULE_NAME,info = "Kernel Basic Verbs Test")
    rc = test.execute()
    sys.exit(rc)
