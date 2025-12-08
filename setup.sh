#!/bin/bash

# Source Xilinx Vitis environment
source /opt/xilinx/2025.2/Vitis/settings64.sh

# Set Versal common image path
export COMMON_IMAGE_VERSAL=/opt/xilinx/xilinx-versal-common-v2025.2

# Set Vitis base platform repository path
export PLATFORM_REPO_PATHS=/opt/xilinx/2025.2/Vitis/base_platforms

echo "Vitis environment for 2025.2 configured."
echo "COMMON_IMAGE_VERSAL = $COMMON_IMAGE_VERSAL"
echo "PLATFORM_REPO_PATHS = $PLATFORM_REPO_PATHS"