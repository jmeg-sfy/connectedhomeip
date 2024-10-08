#!/bin/bash

## ---
# This is a helper script that runs all the yaml tests for all the application clusters enabled in the Closures App.
# Run the script from the root dir.
# The script takes the node ID that the device was commissioned with.
## ---

NODEID=$1
CLOSURES_DEVICE_ENDPOINT=1
CLOSURES_DEVICE_PICS="examples/closures-app/closures-common/pics/closures-app-pics-values"

if [ -z "$NODEID" ]; then
    echo "Usage: run_all_yaml_tests [Node ID]"
    exit
fi

# Closures Operational State cluster
./scripts/tests/chipyaml/chiptool.py tests Test_TC_CLOPSTATE_1_1 --PICS "$CLOSURES_DEVICE_PICS" --nodeId "$NODEID" --endpoint "$CLOSURES_DEVICE_ENDPOINT" &&
    echo done
