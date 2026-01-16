#!/bin/bash

#Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

unset LD_LIBRARY_PATH

# =======================================================
# Set Platform ,Vitis and Versal Image repo
# =======================================================
export PLATFORM_REPO_PATHS=/opt/xilinx/2025.2/Vitis/base_platforms
export XILINX_VITIS=/opt/xilinx/2025.2/Vitis
export COMMON_IMAGE_VERSAL=/opt/xilinx/xilinx-versal-common-v2025.2
# ====================================================
# Source Versal Image ,Vitis and Aietools
# ====================================================
# Run the below command to setup environment and CXX
source $COMMON_IMAGE_VERSAL/environment-setup-cortexa72-cortexa53-amd-linux
source $XILINX_VITIS/settings64.sh

# =========================================================
# Platform Selection...
# =========================================================
tgt_plat=xilinx_vck190_base_202520_1
export PLATFORM=$PLATFORM_REPO_PATHS/$tgt_plat/$tgt_plat\.xpfm
# ==========================================================
# Validating Tool Installation
# ==========================================================
echo ""
echo "Aiecompiler:"
which aiecompiler
echo ""
echo "Vivado:"
which vivado
echo ""
echo "Vitis:"
which vitis
echo ""
echo "Vitis HLS:"
which vitis_hls
echo ""

