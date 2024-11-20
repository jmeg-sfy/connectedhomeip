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
from chip.testing.matter_testing import MatterBaseTest, TestStep, async_test_body, default_matter_test_main, type_matches
from mobly import asserts
import time
from TC_ClosureCommonOperations import ClosureCommonOperations


class TC_CLOPSTATE_2_3(MatterBaseTest):

    def __init__(self, *args):
        super().__init__(*args)
        self.is_ci = False
        self.app_pipe = "/tmp/chip_closure_fifo_"
        self.common_ops = ClosureCommonOperations(self)

    def desc_TC_CLOPSTATE_2_3(self) -> str:
        """Returns a description of this test"""
        return "[TC-CLOPSTATE-2.3] Stop command"

    def pics_TC_CLOPSTATE_2_3(self) -> list[str]:
        return ["CLOPSTATE.S"]

    def steps_TC_CLOPSTATE_2_3(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=False),
            TestStep("2a", "Put the device in setup required operational state"),
            TestStep("2b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("2c", "TH sends Stop command to the DUT"),
            TestStep("3a", "Put the device in stopped operational state"),
            TestStep("3b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("3c", "TH sends Stop command to the DUT"),
            TestStep("4a", "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured, Speed=High to the DUT"),
            TestStep("4b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds"),
            TestStep("4c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("5a", "TH sends MoveTo command with fields Tag=OpenInFull, Latch=NotLached, Speed=Low to the DUT"),
            TestStep("5b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds"),
            TestStep("5c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("5d", "TH sends Stop command to the DUT"),
            TestStep("5e", "TH reads from the DUT the OperationalState attribute"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_2_3(self):

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

        # Check a mandatory Invalid State behavior
        # STEP 2a
        self.step("2a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_SETUPREQUIRED")):
            self.print_step("step number 2a", "Put the device in setup required operational state")
            self.write_to_app_pipe({"Name": "SetSetupRequired", "SetupRequired": True})

            # STEP 2b
            self.step("2b")
            self.print_step("step number 2b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(0x43)
        else:
            self.skip_step("2b")
            logging.info("Step test 2b skipped. The Dut need to be put in setup required state")

        # STEP 2c
        self.step("2c")
        self.print_step("step number 2c", "TH sends Stop command to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Stop()
        expected_response = Status.InvalidInState
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # Check Stop already in Stopped state
        # STEP 3a: TH get the DUT into Stopped state
        self.step("3a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_STOPPED")):
            self.print_step("step number 3a", "Put the device in stop operational state")
            self.write_to_app_pipe({"Name": "Stopped"})

            # STEP 3b
            self.step("3b")
            self.print_step("step number 3b", "Read and check the operational state attribute")
            await self.common_ops.read_operational_state_with_check(0x00)
        else:
            self.skip_step("3b")
            logging.info("Step test 3b skipped. The Dut need to be put in setup required state")

        # STEP 3c
        self.step("3c")
        self.print_step("step number 3c", "TH sends Stop command to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Stop()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # Check Stop from a Paused/Running state (Positioning and/or Latching)
        # Fast PrePositioning to Closed / PreLatching to Secured
        # STEP 4a
        self.step("4a")
        self.print_step("step number 4a",
                        "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured, Speed=High to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kHigh)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4b
        self.step("4b")
        self.print_step("step number 4b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds")
        time.sleep(11)  # TODO Use pics value

        # STEP 4c
        self.step("4c")
        self.print_step("step number 4c", "Read and check the operational state attribute, Verify that the DUT response is Stopped(0x00)")
        await self.common_ops.read_operational_state_with_check(0x00)

        # Slow Positioning toward Open / UnLatch
        # STEP 5a
        self.step("5a")
        self.print_step("step number 5a",
                        "TH sends MoveTo command with fields Tag=OpenInFull, Latch=NotLatched, Speed=Low to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInFull,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kNotLatched,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kLow)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5b
        self.step("5b")
        self.print_step("step number 5b", "TH waits for PIXIT.CLOPSTATE.HalfMotionDelay  seconds")
        time.sleep(1)  # TODO Use pics value

        # STEP 5c
        self.step("5c")
        self.print_step(
            "step number 5c", "Read and check the operational state attribute, Verify that the DUT response is Running(0x01)")
        await self.common_ops.read_operational_state_with_check(0x01)

        # STEP 5d
        self.step("5d")
        self.print_step("step number 5d", "TH sends Stop command to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.Stop()
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5e
        self.step("5e")
        self.print_step("step number 5e", "Read and check the operational state attribute, Verify that the DUT response is Stopped(0x00)")
        await self.common_ops.read_operational_state_with_check(0x00)


if __name__ == "__main__":
    default_matter_test_main()
