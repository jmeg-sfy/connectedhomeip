# closure example app

This example app is meant to demonstrate an implementation of a Matter Closure
Device.

## State machine

Below is a diagram describing the state machine for this app.

![state machine](closure_app_state_diagram.png)

This app can support most of the tests in the test plans.

## Out-of-band messages

Out-of-band messages are available to simulate typical device behaviors and
allow the app to navigate to all the states. To send an out-of-band message,
echo the JSON message to the `/tmp/chip_closure_fifo_<PID>` file. The JSON
message must have a `"Name"` key that contains the command name. This name is
shown in the state machine diagram above. Example
`echo '{"Name": "Charged"}' > /tmp/chip_closure_fifo_42`.

#### `MoveTo` message

#### `Stop` message

#### `ConfigureFallback` message

#### `CancelFallback` message

#### `Calibrate` message

#### `Calibrate` message

#### `Protected` message

#### `UnProtected` message

#### `SetReadyToRun` message

#### `SetActionNeeded` message

#### `SetSetupRequired` message

#### `ErrorEvent` message

#### `ClearError` message

#### `Reset` message

## Testing

A PICS file that details what this app supports testing is available in the
`pics` directory as a txt file. After building the closure example app,
chip-tool, and setting up the testing environment, python tests can be executed
with
`./scripts/tests/run_python_test.py --script src/python_testing/<script_name>.py --script-args "--storage-path admin_storage.json --PICS examples/closure-app/closure-common/pics/closure_App_Test_Plan.txt --int-arg <PIXIT_Definitions:1>"`

**Note:** If the testing environment has not been commissioned with the closure
app,

1. use chip-tool to switch on the commissioning window
   `out/debug/chip-tool pairing open-commissioning-window 0x1230 1 180 1000 42`
2. Get the manual pairing code. This will look something like
   `Manual pairing code: [01073112097]`.
3. Run any one of the tests with the `--commission-only` and `--manual-code`
   flags:
   `./scripts/tests/run_python_test.py --script src/python_testing/TC_CLOPSTATE-2_1.py --script-args "--commissioning-method on-network --manual-code 01073112097 --commission-only"`

Below are the PIXIT definitions required for the different python tests.

#### TC 1.2

#### TC 2.1

#### TC 2.2
