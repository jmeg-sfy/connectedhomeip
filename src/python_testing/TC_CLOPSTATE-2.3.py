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


# Takes an OperationState or ClosureOperationalState state enum and returns a string representation
def state_enum_to_text(state_enum):
    if state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kStopped:
        return "Stopped(0x00)"
    elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kRunning:
        return "Running(0x01)"
    else:
        return "UnknownEnumValue"


class TC_CLOPSTATE_2_3(MatterBaseTest):

    def __init__(self, *args):
        super().__init__(*args)
        self.is_ci = False
        self.app_pipe = "/tmp/chip_closure_fifo_"

    # Prints the step number, reads the operational state attribute and checks if it matches with expected_state
    async def read_operational_state_with_check(self, expected_state):
        logging.info("Read Operational state")
        operational_state = await self.read_single_attribute_check_success(
            endpoint=self.endpoint, cluster=Clusters.Objects.ClosureOperationalState, attribute=Clusters.ClosureOperationalState.Attributes.OperationalState)
        logging.info("OperationalState: %s" % operational_state)
        asserts.assert_equal(operational_state, expected_state,
                             "OperationalState(%s) should be %s" % (operational_state, state_enum_to_text(expected_state)))

    def desc_TC_CLOPSTATE_2_3(self) -> str:
        """Returns a description of this test"""
        return "[TC-CLOPSTATE-2.3] Stop command"

    def pics_TC_CLOPSTATE_2_3(self) -> list[str]:
        return ["CLOPSTATE.S"]

    def steps_TC_CLOPSTATE_2_3(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=False),
            TestStep(2, "Put the DUT into Running state"),
            TestStep(3, "Read the OperationalState attribute"),
            TestStep(4, "Send Stop command"),
            TestStep(5, "Read the OperationalState attribute"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_2_3(self):

        self.endpoint = self.matter_test_config.endpoint
        asserts.assert_false(self.endpoint is None, "--endpoint <endpoint> must be included on the command line in.")
        self.is_ci = self.check_pics("PICS_SDK_CI_ONLY")
        self.is_ci = True
        if self.is_ci:
            app_pid = self.matter_test_config.app_pid
            if app_pid == 0:
                asserts.fail("The --app-pid flag must be set when PICS_SDK_CI_ONLY is set")
            self.app_pipe = self.app_pipe + str(app_pid)

        # STEP 1: Commission DUT (already done)
        self.step(1)

        # STEP 2: TH get the DUT into Running state
        self.step(2)
        if self.is_ci:
            # CI call to trigger a movement and go to running state.
            self.print_step("step number 2", "Send MoveTo command to pipe")
            self.write_to_app_pipe({"Name": "MoveTo", "Tag": 3, "Speed": 1, "Latch": 0})
        else:
            self.print_step("step number 2", "Send MoveTo command")
            tag = Clusters.Objects.ClosureOperationalState.Enums.TagEnum.kOpenInHalf
            try:
                await self.send_single_cmd(cmd=Clusters.Objects.ClosureOperationalState.Commands.MoveTo(tag), endpoint=self.endpoint)
            except InteractionModelError as e:
                asserts.assert_equal(e.status, Status.Success, "Unexpected error returned")

        self.step(3)
        # Ensure that the device is in the correct state: Stopped (0x01)
        self.print_step("step number 3", "Ensure that devices is in Running state")
        # TODO Add expected status enum
        await self.read_operational_state_with_check(0x01)

        # STEP 3: TH sends the stop command to the DUT
        self.step(4)
        self.print_step("step number 4", "Send Stop command")
        try:
            await self.send_single_cmd(cmd=Clusters.Objects.ClosureOperationalState.Commands.Stop(), endpoint=self.endpoint)
        except InteractionModelError as e:
            asserts.assert_equal(e.status, Status.Success, "Unexpected error returned")
        # await self.send_stop_cmd_with_check()

        # STEP 4: TH reads the Operational state attribute
        self.step(5)
        self.print_step("step number 5", "Ensure that devices is in Stopped state")
        # Ensure that the device is in the correct state: Stopped (0x00)
        await self.read_operational_state_with_check(0x00)


if __name__ == "__main__":
    default_matter_test_main()
