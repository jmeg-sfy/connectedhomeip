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
    elif state_enum == Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kSetupRequired:
        return "SetupRequired(0x43)"
    else:
        return "UnknownEnumValue"


class TC_CLOPSTATE_2_2(MatterBaseTest):

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

    async def send_cmd_expect_response(self, endpoint, cmd, expected_response_status, timedRequestTimeoutMs=None):
        try:
            await self.send_single_cmd(endpoint=endpoint,
                                       cmd=cmd,
                                       timedRequestTimeoutMs=timedRequestTimeoutMs)
        except InteractionModelError as e:
            asserts.assert_equal(e.status,
                                 expected_response_status,
                                 f"Command response ({e.status}) mismatched from expected status {expected_response_status} for {cmd} on {endpoint}")

    def desc_TC_CLOPSTATE_2_2(self) -> str:
        """Returns a description of this test"""
        return "[TC-CLOPSTATE-2.2] MoveTo Command Inputs Sanity Check with DUT as Server"

    def pics_TC_CLOPSTATE_2_2(self) -> list[str]:
        return ["CLOPSTATE.S"]

    def steps_TC_CLOPSTATE_2_2(self) -> list[TestStep]:
        steps = [
            TestStep(1, "Commissioning, already done", is_commissioning=False),
            TestStep(2, "TH sends MoveTo command with no field"),
            TestStep("3a", "Put the device in setup required operational state"),
            TestStep("3b", "TH reads from the DUT the OperationalState attribute"),
            TestStep("3c", "TH sends command with only field Tag=CloseInFull"),
            TestStep("4a", "TH sends MoveTo command with only field Tag=CloseInFull"),
            TestStep("4b", "TH sends MoveTo command with only field Tag=OpenInFull"),
            TestStep("4c", "TH sends MoveTo command with only field Tag=OpenOneQuarter"),
            TestStep("4d", "TH sends MoveTo command with only field Tag=OpenInHalf"),
            TestStep("4e", "TH sends MoveTo command with only field Tag=Pedestrian"),
            TestStep("4f", "TH sends MoveTo command with only field Tag=Ventilation"),
            TestStep("4g", "TH sends MoveTo command with only field Tag=Signature"),
            TestStep("4h", "TH sends MoveTo command with only field Tag=SequenceNextStep"),
            TestStep("4i", "TH sends MoveTo command with only field Tag=PedestrianNextStep"),
            TestStep("4j", "TH sends MoveTo command with only field Tag=TagInvalid"),
            TestStep("5a", "TH sends MoveTo command with only the Latch field set to LatchedAndSecured"),
            TestStep("5b", "TH sends MoveTo command with only the Latch field set to not allowed LatchedButNotSecured"),
            TestStep("5c", "TH sends MoveTo command with only the Latch field set to NotLatched"),
            TestStep("5d", "TH sends MoveTo command with only the Latch field set to LatchInvalid"),
            TestStep("6a", "TH sends MoveTo command with only the Speed field set to Auto"),
            TestStep("6b", "TH sends MoveTo command with only the Speed field set to Low"),
            TestStep("6c", "TH sends MoveTo command with only the Speed field set to Medium"),
            TestStep("6d", "TH sends MoveTo command with only the Speed field set to High"),
            TestStep("6e", "TH sends MoveTo command with only the Speed field set to SpeedInvalid"),
            TestStep("7a", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured"),
            TestStep("7b", "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic"),
            TestStep("7c", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured"),
            TestStep("7d", "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic"),
            TestStep("7e", "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=Automatic"),
            TestStep("7f", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatInvalid"),
            TestStep("7g", "TH sends MoveTo command with fields Tag=Pedestrian, Speed=SpdInvalid"),
            TestStep("7h", "TH sends MoveTo command with fields Tag=Ventilation, Latch=LatInvalid"),
            TestStep("7i", "TH sends MoveTo command with fields Tag=TagInvalid, Speed=Automatic"),
            TestStep("7j", "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=SpdInvalid"),
            TestStep("7k", "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured, Speed=Automatic"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_2_2(self):

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

        # STEP 2: TH sends MoveTo command with no field
        self.step(2)
        if self.is_ci:
            self.print_step("step number 2", "Send MoveTo command with no field to pipe")
            self.write_to_app_pipe({"Name": "MoveTo"})
        else:
            self.print_step("step number 2", "Send MoveTo command with no field")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo()
            expected_response = Status.InvalidCommand
            await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 3a: Put the device in setup required operational state
        self.step("3a")
        if self.is_ci:
            self.print_step("step number 3a", "Put the device in setup required operational state")
            self.write_to_app_pipe({"Name": "SetSetupRequired", "SetupRequired": True})

        # STEP 3b: Put the device in setup required operational state
        self.step("3b")
        await self.read_operational_state_with_check(0x43)

        # STEP 3c: TH sends command with only field Tag=CloseInFull
        self.step("3c")
        self.print_step("step number 3c", "Send MoveTo command with tag field equals to CloseInFull")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull)
        expected_response = Status.InvalidInState
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # Reset DUT
        if self.is_ci:
            self.write_to_app_pipe({"Name": "Reset"})

        # STEP 4a: TH sends MoveTo command with only field Tag=CloseInFull
        self.step("4a")
        self.print_step("step number 4a",
                        "TH sends MoveTo command with only field Tag=CloseInFull. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4b: TH sends MoveTo command with only field Tag=OpenInFull
        self.step("4b")
        self.print_step("step number 4a",
                        "TH sends MoveTo command with only field Tag=OpenInFull. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInFull)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4c: TH sends MoveTo command with only field Tag=OpenOneQuarter
        self.step("4c")
        self.print_step("step number 4c",
                        "TH sends MoveTo command with only field Tag=OpenOneQuarter. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenOneQuarter)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4d: TH sends MoveTo command with only field Tag=OpenInHalf
        self.step("4d")
        self.print_step("step number 4d",
                        "TH sends MoveTo command with only field Tag=OpenInHalf. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4e: TH sends MoveTo command with only field Tag=OpenThreeQuarter
        self.step("4e")
        self.print_step("step number 4e",
                        "TH sends MoveTo command with only field Tag=OpenThreeQuarter. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenThreeQuarter)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4f: TH sends MoveTo command with only field Tag=Pedestrian
        self.step("4f")
        self.print_step("step number 4f",
                        "TH sends MoveTo command with only field Tag=Pedestrian. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrian)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4g: TH sends MoveTo command with only field Tag=Ventilation
        self.step("4g")
        self.print_step("step number 4g",
                        "TH sends MoveTo command with only field Tag=Ventilation. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kVentilation)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4h: TH sends MoveTo command with only field Tag=Signature
        self.step("4h")
        self.print_step("step number 4h",
                        "TH sends MoveTo command with only field Tag=Signature. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kSignature)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4i: TH sends MoveTo command with only field Tag=SequenceNextStep
        self.step("4i")
        self.print_step("step number 4i",
                        "TH sends MoveTo command with only field Tag=SequenceNextStep. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kSequenceNextStep)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4j: TH sends MoveTo command with only field Tag=TagInvalid
        self.step("4j")
        self.print_step("step number 4j",
                        "TH sends MoveTo command with only field Tag=TagInvalid. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kUnknownEnumValue)  # TODO
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # TODO add not implemented feature tests (not found)

        # CHECK LATCH FIELD
        # STEP 5a
        self.step("5a")
        self.print_step("step number 5a",
                        "TH sends MoveTo command with only the Latch field set to LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5b
        self.step("5b")
        self.print_step("step number 5b",
                        "TH sends MoveTo command with only the Latch field set to LatchedButNotSecured. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedButNotSecured)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5c
        self.step("5c")
        self.print_step("step number 5c",
                        "TH sends MoveTo command with only the Latch field set to NotLatched. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kNotLatched)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5d
        self.step("5d")
        self.print_step("step number 5d",
                        "TH sends MoveTo command with only the Latch field set to kUnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # CHECK SPEED FIELD
        # STEP 6a
        self.step("6a")
        self.print_step("step number 6a",
                        "TH sends MoveTo command with only the Speed field set to Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6b
        self.step("6b")
        self.print_step("step number 6b",
                        "TH sends MoveTo command with only the Speed field set to Low. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kLow)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6c
        self.step("6c")
        self.print_step("step number 6c",
                        "TH sends MoveTo command with only the Speed field set to Medium. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kMedium)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6d
        self.step("6d")
        self.print_step("step number 6d",
                        "TH sends MoveTo command with only the Speed field set to High. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kHigh)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6e
        self.step("6e")
        self.print_step("step number 6e",
                        "TH sends MoveTo command with only the Speed field set to UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # CHECK MULTIPLE FIELDS
        # STEP 7a
        self.step("7a")
        self.print_step("step number 7a",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7b
        self.step("7b")
        self.print_step("step number 7b",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7c
        self.step("7c")
        self.print_step("step number 7c",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured. Verify DUT responds with status NOT_FOUND(0x8b)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.NotFound
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7d
        self.step("7d")
        self.print_step("step number 7c",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic. Verify DUT responds with status NOT_FOUND(0x8b)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.NotFound
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7e
        self.step("7e")
        self.print_step("step number 7e",
                        "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7f
        self.step("7f")
        self.print_step("step number 7f",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatInvalid. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7g
        self.step("7g")
        self.print_step("step number 7g",
                        "TH sends MoveTo command with fields Tag=Pedestrian, Speed=SpdInvalid. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrian,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7h
        self.step("7h")
        self.print_step("step number 7h",
                        "TH sends MoveTo command with fields Tag=Ventilation, Latch=LatInvalid. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kVentilation,
            speed=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7i
        self.step("7i")
        self.print_step("step number 7i",
                        "TH sends MoveTo command with fields Tag=TagInvalid, Speed=Automatic. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kUnknownEnumValue,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7j
        self.step("7j")
        self.print_step("step number 7j",
                        "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=SpdInvalid. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7e
        self.step("7k")
        self.print_step("step number 7k",
                        "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.send_cmd_expect_response(self.endpoint, cmd, expected_response)


if __name__ == "__main__":
    default_matter_test_main()
