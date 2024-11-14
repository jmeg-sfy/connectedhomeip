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

# Takes an OperationState or ClosureOperationalState state enum and returns a string representation


def state_enum_to_text(state_enum):
    if state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kStopped:
        return "Stopped(0x00)"
    elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kRunning:
        return "Running(0x01)"
    elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kPaused:
        return "Running(0x02)"
    else:
        return "UnknownEnumValue"


class TC_CLOPSTATE_3_2(MatterBaseTest):

    def __init__(self, *args):
        super().__init__(*args)
        self.is_ci = False
        self.app_pipe = "/tmp/chip_closure_fifo_"

    # Prints the step number, reads the operational state attribute and checks if it matches with expected_state
    async def read_operational_state_with_check(self, expected_state):
        operational_state = await self.read_single_attribute_check_success(
            endpoint=self.endpoint, cluster=Clusters.Objects.ClosureOperationalState, attribute=Clusters.ClosureOperationalState.Attributes.OperationalState)
        logging.info(f"OperationalState: {operational_state}")
        asserts.assert_equal(operational_state, expected_state,
                             "OperationalState(%s) should be %s" % (operational_state, state_enum_to_text(expected_state)))

    async def read_overall_state_attr_with_check_of_instance(self):
        attribute_to_check = Clusters.ClosureOperationalState.Attributes.OverallState
        overall_state = await self.read_single_attribute_check_success(
            endpoint=self.endpoint, cluster=Clusters.Objects.ClosureOperationalState, attribute=attribute_to_check)

        logging.info(f"OverallState: {overall_state}")
        asserts.assert_true(isinstance(overall_state, Clusters.ClosureOperationalState.Structs.OverallStateStruct),
                            f"OverallState Read is not an instance of {attribute_to_check}")
        return overall_state

    async def send_cmd_expect_response(self, endpoint, cmd, expected_response_status, timedRequestTimeoutMs=None):
        try:
            await self.send_single_cmd(endpoint=endpoint,
                                       cmd=cmd,
                                       timedRequestTimeoutMs=timedRequestTimeoutMs)
        except InteractionModelError as e:
            asserts.assert_equal(e.status,
                                 expected_response_status,
                                 f"Command response ({e.status}) mismatched from expected status {expected_response_status} for {cmd} on {endpoint}")

    def desc_TC_CLOPSTATE_3_2(self) -> str:
        """Returns a description of this test"""
        return "[TC-CLOPSTATE-3.2] MoveTo Command NextStep behavior with DUT as Server"

    def pics_TC_CLOPSTATE_3_2(self) -> list[str]:
        return ["CLOPSTATE.S"]

    def steps_TC_CLOPSTATE_3_2(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=False),
            TestStep("2a", "Manually put the device in the Stopped(0x00) operational state"),
            TestStep("2b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("3a", "TH sends MoveTo command with fields Tag=CloseInFull(0), Latch=LatchedAndSecured(0), Speed=High(3) to the DUT"),
            TestStep("3b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds"),
            TestStep("3c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("4a", "TH reads from the DUT the OverallState attribute"),
            TestStep("4b", "Check field tmpOverallState.Positioning"),
            TestStep("4c", "Check field tmpOverallState.Latching"),
            TestStep("5a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8), Latch=NotLatched(2), Speed=High(3) to the DUT"),
            TestStep("5b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds"),
            TestStep("5c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("6a", "TH reads from the DUT the OverallState attribute"),
            TestStep("6b", "Check field tmpOverallState.Positioning"),
            TestStep("6c", "Check field tmpOverallState.Latching"),
            TestStep("7a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8), Latch=LatchedAndSecured(0), Speed=High(3) to the DUT"),
            TestStep("7b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds"),
            TestStep("7c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("8a", "TH reads from the DUT the OverallState attribute"),
            TestStep("8b", "Check field tmpOverallState.Positioning"),
            TestStep("8c", "Check field tmpOverallState.Latching"),
            TestStep("9a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8), Speed=Low(1) to the DUT"),
            TestStep("9b", "TH waits for PIXIT.CLOPSTATE.HalfMotionDelay seconds"),
            TestStep("9c", "TH reads from the DUT the OperationalState attribute"),
            TestStep("10a", "TH reads from the DUT the OverallState attribute"),
            TestStep("10b", "Check field tmpOverallState.Positioning"),
            TestStep("11a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8) to the DUT"),
            TestStep("11b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("12a", "TH reads from the DUT the OverallState attribute"),
            TestStep("12b", "Check field tmpOverallState.Positioning"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_3_2(self):

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

        # STEP 2a: TH get the DUT into Stopped state
        self.step("2a")
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_STOPPED")):
            self.print_step("step number 2a", "Put the device in stop operational state")
            self.write_to_app_pipe({"Name": "Stopped"})

            # STEP 2b
            self.step("2b")
            self.print_step("step number 2b", "Read and check the operational state attribute")
            await self.read_operational_state_with_check(0x00)
        else:
            self.skip_step("2b")
            logging.info("Step test 2b skipped. The Dut need to be put in setup required state")

        # Check OverallState from a Stopped state
        # Fast PrePositioning to FullyClosed / PreLatching to Secured
        # STEP 3a
        self.step("3a")
        self.print_step("step number 3a",
                        "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured, Speed=High to the DUT")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kHigh)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 3b
        self.step("3b")
        self.print_step("step number 3b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds")
        time.sleep(11)  # TODO Use pics value

        # STEP 3c
        self.step("3c")
        self.print_step("step number 3c", "Read and check the operational state attribute, Verify that the DUT response is Stopped(0x00)")
        await self.read_operational_state_with_check(0x00)

        # STEP 4a
        self.step("4a")
        self.print_step("step number 4a", "TH reads from the DUT the OverallState attribute")
        read_attr = await self.read_overall_state_attr_with_check_of_instance()

        # STEP 4b
        self.step("4b")
        self.print_step("step number 4b", "Check field tmpOverallState.Positioning")
        asserts.assert_equal(read_attr.positioning, Clusters.ClosureOperationalState.Enums.PositioningEnum.kFullyClosed)

        # STEP 4c
        self.step("4c")
        self.print_step("step number 4c", "Check field tmpOverallState.Latching")
        # STEP 5a
        self.step("5a")
        self.print_step(
            "step number 5a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8), Latch=NotLatched(2), Speed=High(3) to the DUT")
        # STEP 5b
        self.step("5b")
        self.print_step("step number 5b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds")
        # STEP 5c
        self.step("5c")
        self.print_step("step number 5c", "TH reads from the DUT the OperationalState attribute")
        # STEP 6a
        self.step("6a")
        self.print_step("step number 6a", "TH reads from the DUT the OverallState attribute")
        # STEP 6b
        self.step("6b")
        self.print_step("step number 6b", "Check field tmpOverallState.Positioning")
        # STEP 6c
        self.step("6c")
        self.print_step("step number 6c", "Check field tmpOverallState.Latching")
        # STEP 7a
        self.step("7a")
        self.print_step(
            "step number 7a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8), Latch=LatchedAndSecured(0), Speed=High(3) to the DUT")
        # STEP 7b
        self.step("7b")
        self.print_step("step number 7b", "TH waits for PIXIT.CLOPSTATE.FullMotionDelay seconds")
        # STEP 7c
        self.step("7c")
        self.print_step("step number 7c", "TH reads from the DUT the OperationalState attribute")
        # STEP 8a
        self.step("8a")
        self.print_step("step number 8a", "TH reads from the DUT the OverallState attribute")
        # STEP 8b
        self.step("8b")
        self.print_step("step number 8b", "Check field tmpOverallState.Positioning")
        # STEP 8c
        self.step("8c")
        self.print_step("step number 8c", "Check field tmpOverallState.Latching")
        # STEP 9a
        self.step("9a")
        self.print_step("step number 9a", "TH reads from the DUT the OverallState attribute")
        # STEP 9b
        self.step("9b")
        self.print_step("step number 9b", "TH waits for PIXIT.CLOPSTATE.HalfMotionDelay seconds")
        # STEP 9c
        self.step("9c")
        self.print_step("step number 9c", "TH reads from the DUT the OperationalState attribute")
        # STEP 10a
        self.step("10a")
        self.print_step("step number 10a", "TH reads from the DUT the OverallState attribute")
        # STEP 10b
        self.step("10b")
        self.print_step("step number 10b", "Check field tmpOverallState.Positioning")
        # STEP 11a
        self.step("11a")
        self.print_step("step number 11a", "TH sends MoveTo command with fields Tag=SequenceNextStep(8) to the DUT")
        # STEP 11b
        self.step("11b")
        self.print_step("step number 11b", "TH reads from the DUT the OperationalState attribute")
        # STEP 12a
        self.step("12a")
        self.print_step("step number 12a", "TH reads from the DUT the OverallState attribute")
        # STEP 12b
        self.step("12b")
        self.print_step("step number 12b", "Check field tmpOverallState.Positioning")


if __name__ == "__main__":
    default_matter_test_main()
