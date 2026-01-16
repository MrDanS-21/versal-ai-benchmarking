#!/bin/bash

echo ""
date
echo ""

start=${EPOCHREALTIME}

# Executing the elf...
./gemm_arm.elf

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
