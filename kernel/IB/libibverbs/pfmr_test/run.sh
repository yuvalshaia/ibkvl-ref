#!/bin/bash

# For complete end-toend test run:
#mlxkvl stop;mlxkvl start && insmod pfmr_ktest.ko
#mlxkvl mlx4_0-complete_test

mlxkvl stop;mlxkvl start && insmod pfmr_ktest.ko
mlxkvl mlx4_0-create_fmr nfmr=2
mlxkvl mlx4_0-create_dma ndma=2
mlxkvl mlx4_0-fmr_map fmr_idx=1 dma_idx=1
mlxkvl mlx4_0-fmr_map fmr_idx=2 dma_idx=2
mlxkvl mlx4_0-dma dma_idx=1 set=data11111
mlxkvl mlx4_0-dma dma_idx=2 set=data22222
mlxkvl mlx4_0-cq cr=1
mlxkvl mlx4_0-cq cr=1
mlxkvl mlx4_0-qp cr=1 scq_idx=1 rcq_idx=1
mlxkvl mlx4_0-qp cr=1 scq_idx=2 rcq_idx=2
mlxkvl mlx4_0-qp show=1
#mlxkvl mlx4_0-qp rts=1 port=1 dqpn_idx=2
#mlxkvl mlx4_0-qp rts=2 port=2 dqpn_idx=1
mlxkvl mlx4_0-qp rts=1 port=1 dqpn_idx=1
mlxkvl mlx4_0-create_dma show=1
mlxkvl mlx4_0-fmr_map show=1
mlxkvl mlx4_0-qp rdma=1 fmr_idx=1 rfmr_idx=2
mlxkvl mlx4_0-dma get=1 dma_idx=1
mlxkvl mlx4_0-dma get=1 dma_idx=2
