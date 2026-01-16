#!/bin/bash
#Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

echo ""
date
echo ""

start=${EPOCHREALTIME}

#export XLC_EMULATION_MODE=hw_emu
export XILINX_XRT=/usr

# Executing the elf...
./gemm_dsp_xrt.elf a.xclbin

return_code=$?

end=${EPOCHREALTIME}

elapsed=$(awk "BEGIN {print $end - $start}")

if [ $return_code -ne 0 ]; then
        echo "ERROR: Embedded host run failed, RC=$return_code"
else
        echo "INFO: TEST PASSED, RC=0"
fi

ms=$(awk "BEGIN {printf \"%.3f\", ($elapsed*1000)}")
printf "\nElapsed time: %s ms\n" "$ms"

date
echo ""
echo "INFO: Embedded host run completed."
echo ""

exit $return_code
