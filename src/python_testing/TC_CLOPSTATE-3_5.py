#
#    Copyright (c) 2024 Project CHIP Authors
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

# See https://github.com/project-chip/connectedhomeip/blob/master/docs/testing/python.md#defining-the-ci-test-arguments
# for details about the block below.
#
# === BEGIN CI TEST ARGUMENTS ===
# test-runner-runs: run1
# test-runner-run/run1/app: ${ALL_CLUSTERS_APP}
# test-runner-run/run1/factoryreset: True
# test-runner-run/run1/quiet: True
# test-runner-run/run1/app-args: --discriminator 1234 --KVS kvs1 --trace-to json:${TRACE_APP}.json
# test-runner-run/run1/script-args: --storage-path admin_storage.json --commissioning-method on-network --discriminator 1234 --passcode 20202021 --trace-to json:${TRACE_TEST_JSON}.json --trace-to perfetto:${TRACE_TEST_PERFETTO}.perfetto
# === END CI TEST ARGUMENTS ===
#

import logging
import chip.clusters as Clusters
from chip.interaction_model import InteractionModelError, Status
from chip.testing.matter_testing import MatterBaseTest, TestStep, async_test_body, default_matter_test_main
from mobly import asserts
import time
from TC_ClosureCommonOperations import ClosureCommonOperations


class TC_CLOPSTATE_3_5(MatterBaseTest):

    def __init__(self, *args):
        super().__init__(*args)
        self.is_ci = False
        self.app_pipe = "/tmp/chip_closure_fifo_"
        self.common_ops = ClosureCommonOperations(self)

    def desc_TC_CLOPSTATE_3_5(self) -> str:
        """Returns a description of this test"""
        return "[TC-CLOPSTATE-3.5] Calibrate Command and State with DUT as Server"

    def pics_TC_CLOPSTATE_3_5(self) -> list[str]:
        return ["CLOPSTATE.S"]

    def steps_TC_CLOPSTATE_3_5(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=False),
            TestStep("2a", "Manually put the device in the SetupRequired(0x43) operational state"),
            TestStep("2b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("2c", "TH sends Calibrate command with any field to the DUT"),
            TestStep("3a", "Manually put the device in the Stopped(0x00) operational state"),
            TestStep("3b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("3c", "TH sends Calibrate command to the DUT"),
            TestStep("3d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds"),
            TestStep("3e", "TH reads from the DUT the OperationalState attribute"),
            TestStep("4a", "TH sends Calibrate command to the DUT"),
            TestStep("4b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("5a", "TH waits for PIXIT.CLOPSTATE.ConcludeCalibrationDelay seconds or Manually put the device to Stopped(0x00) operational state"),
            TestStep("5b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("5c", "TH reads from the DUT the OperationalError attribute"),
            TestStep("6a", "Manually put the device in the Stopped(0x00) operational state"),
            TestStep("6b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("6c", "TH sends Calibrate command to the DUT"),
            TestStep("6d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds"),
            TestStep("6e", "TH reads from the DUT the OperationalState attribute"),
            TestStep("7a", "TH sends Stop command to the DUT"),
            TestStep("7b", "TH waits for PIXIT.CLOPSTATE.AbortingCalibrationDelay seconds"),
            TestStep("7c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("7d", "TH reads from the DUT the OperationalError attribute"),
            TestStep("8a", "Manually put the device in the Stopped(0x00) operational state"),
            TestStep("8b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("8c", "TH sends Calibrate command to the DUT"),
            TestStep("8d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds"),
            TestStep("8e", "TH reads from the DUT the OperationalState attribute"),
            TestStep("9a", "Introduce a calibration error or Manually put the device in the Error(0x03) operational state"),
            TestStep("9b", "TH waits for PIXIT.CLOPSTATE.AbortingCalibrationDelay seconds"),
            TestStep("9c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("9d", "TH reads from the DUT the OperationalError attribute"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_3_5(self):

        self.endpoint = self.matter_test_config.endpoint
        asserts.assert_false(self.endpoint is None, "--endpoint <endpoint> must be included on the command line in.")
        self.is_ci = self.check_pics("PICS_SDK_CI_ONLY")

        if self.is_ci:
            app_pid = self.matter_test_config.app_pid
            if app_pid == 0:
                asserts.fail("The --app-pid flag must be set when PICS_SDK_CI_ONLY is set")
            self.app_pipe = self.app_pipe + str(app_pid)

        # STEP 1: Commission DUT (already done)
        self.step(1)

        # STEP 2a
        self.step("2a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_SETUPREQUIRED")):
            self.print_step("step number 2a", "Put the device in setup required operational state")
            self.write_to_app_pipe({"Name": "SetSetupRequired", "SetupRequired": True})

            # STEP 2b
            self.step("2b")
            self.print_step("step number 3b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(0x43)
        else:
            self.skip_step("2b")
            logging.info("Step test 2b skipped. The Dut need to be put in setup required state")

        # STEP 2c
        self.step("2c")
        self.print_step(
            "step number 2c", "TH sends Calibrate command with any field to the DUT, Verify DUT responds with status INVALID_IN_STATE(0xcb)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Calibrate()
        expected_response = Status.InvalidInState
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # Start a Calibration
        # STEP 3a
        self.step("3a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_STOPPED")):
            self.print_step("step number 3a", "Put the device in stop operational state")
            self.write_to_app_pipe({"Name": "Stopped"})

            # STEP 3b
            self.step("3b")
            self.print_step("step number 3b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(Clusters.OperationalState.Enums.OperationalStateEnum.kStopped)
        else:
            self.skip_step("3b")
            logging.info("Step test 3b skipped. The Dut need to be put in setup required state")

        # STEP 3c
        self.step("3c")
        self.print_step(
            "step number 3c", "TH sends Calibrate command with any field to the DUT, Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Calibrate()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 3d
        self.step("3d")
        self.print_step("step number 3d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds")
        time.sleep(1)  # TODO Use pics value

        # STEP 3e
        self.step("3e")
        self.print_step(
            "step number 3e", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Calibrating(0x40)")
        await self.common_ops.read_operational_state_with_check(Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating)

        # Check a mandatory Valid State behavior
        # STEP 4a
        self.step("4a")
        self.print_step(
            "step number 4a", "TH sends Calibrate command to the DUT, Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Calibrate()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4b
        self.step("4b")
        self.print_step(
            "step number 4b", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Calibrating(0x40)")
        await self.common_ops.read_operational_state_with_check(Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating)

        # Calibration Success flow (Performed Manually or Automated Calibration)
        # STEP 5a
        self.step("5a")
        self.print_step("step number 5a", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds")
        time.sleep(1)  # TODO Use pics value

        # STEP 5b
        self.step("5b")
        self.print_step(
            "step number 5b", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Stopped(0x00)")
        await self.common_ops.read_operational_state_with_check(Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating)

        # STEP 5c
        self.step("5c")
        self.print_step(
            "step number 5c", "TH reads from the DUT the OperationalError attribute, Verify that the DUT response contains an instance of ErrorStateStruct, ErrorStateId Field Value has to be NoError(0x00)")
        expected_response = Clusters.OperationalState.Enums.ErrorStateEnum.kNoError
        await self.common_ops.read_operational_error_with_check(expected_response)

        # Restart a Calibration
        # STEP 6a
        self.step("6a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_STOPPED")):
            self.print_step("step number 6a", "Put the device in stop operational state")
            self.write_to_app_pipe({"Name": "Stopped"})

            # STEP 6b
            self.step("6b")
            self.print_step("step number 6b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(Clusters.OperationalState.Enums.OperationalStateEnum.kStopped)
        else:
            self.skip_step("6b")
            logging.info("Step test 6b skipped. The Dut need to be put in setup required state")

        # STEP 6c
        self.step("6c")
        self.print_step(
            "step number 6c", "TH sends Calibrate command with any field to the DUT, Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Calibrate()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6d
        self.step("6d")
        self.print_step("step number 6d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds")
        time.sleep(1)  # TODO Use pics value

        # STEP 6e
        self.step("6e")
        self.print_step(
            "step number 6e", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Calibrating(0x40)")
        await self.common_ops.read_operational_state_with_check(Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating)

        # Calibration Abortion flow
        # STEP 7a
        self.step("7a")
        self.print_step(
            "step number 7a", "TH sends Stop command to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Stop()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7b
        self.step("7b")
        self.print_step("step number 7b", "TH waits for PIXIT.CLOPSTATE.AbortingCalibrationDelay seconds")
        time.sleep(5)  # TODO Use pics value

        # STEP 7c
        self.step("7c")
        self.print_step(
            "step number 7c", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Stopped(0x00)")
        await self.common_ops.read_operational_state_with_check(Clusters.OperationalState.Enums.OperationalStateEnum.kStopped)

        # STEP 7d
        self.step("7d")
        self.print_step(
            "step number 7d", "TH reads from the DUT the OperationalError attribute, Verify that the DUT response contains an instance of ErrorStateStruct, ErrorStateId Field Value has to be NoError(0x00)")
        expected_response = Clusters.OperationalState.Enums.ErrorStateEnum.kNoError
        await self.common_ops.read_operational_error_with_check(expected_response)

        # Restart a Calibration
        # STEP 8a
        self.step("8a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_STOPPED")):
            self.print_step("step number 8a", "Put the device in stop operational state")
            self.write_to_app_pipe({"Name": "Stopped"})

            # STEP 8b
            self.step("8b")
            self.print_step("step number 8b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(Clusters.OperationalState.Enums.OperationalStateEnum.kStopped)
        else:
            self.skip_step("8b")
            logging.info("Step test 6b skipped. The Dut need to be put in setup required state")

        # STEP 8c
        self.step("8c")
        self.print_step(
            "step number 8c", "TH sends Calibrate command with any field to the DUT, Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Calibrate()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 8d
        self.step("8d")
        self.print_step("step number 8d", "TH waits for PIXIT.CLOPSTATE.InitiateCalibrationDelay seconds")
        time.sleep(1)  # TODO Use pics value

        # STEP 8e
        self.step("8e")
        self.print_step(
            "step number 6e", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Calibrating(0x40)")
        await self.common_ops.read_operational_state_with_check(Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating)

        # Calibration Failure flow
        # STEP 9a
        self.step("9a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_ERROR")):  # TODO Define pics for manual error
            self.print_step("step number 9a", "Put the device in error operational state")
            self.write_to_app_pipe({"Name": "ErrorEvent", "Error": "Blocked"})

        # STEP 9b
        self.step("9b")
        self.print_step("step number 9b", "TH waits for PIXIT.CLOPSTATE.AbortingCalibrationDela seconds")
        time.sleep(6)  # TODO Use pics value

        # STEP 9c
        self.step("9c")
        self.print_step(
            "step number 9c", "TH reads from the DUT the OperationalState attribute, Verify that the DUT response, the value has to be Error(0x03)")
        await self.common_ops.read_operational_state_with_check(Clusters.OperationalState.Enums.OperationalStateEnum.kError)

        # STEP 9d
        self.step("9d")
        self.print_step(
            "step number 9d", "TH reads from the DUT the OperationalError attribute, Verify that the DUT response contains an instance of ErrorStateStruct, ErrorStateId Field Value has to be NoError(0x00)")
        expected_response = Clusters.OperationalState.Enums.ErrorStateEnum.kNoError
        await self.common_ops.read_operational_error_with_check(expected_response)


if __name__ == "__main__":
    default_matter_test_main()
