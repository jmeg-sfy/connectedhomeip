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
from TC_ClosureCommonOperations import ClosureCommonOperations


class TC_CLOPSTATE_2_2(MatterBaseTest):

    def __init__(self, *args):
        super().__init__(*args)
        self.is_ci = False
        self.app_pipe = "/tmp/chip_closure_fifo_"
        self.common_ops = ClosureCommonOperations(self)

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
            TestStep("3c", "TH sends MoveTo command with only Tag field set to CloseInFull"),
            TestStep("4a", "TH sends MoveTo command with only Tag field set to CloseInFull"),
            TestStep("4b", "TH sends MoveTo command with only Tag field set to OpenInFull"),
            TestStep("4c", "TH sends MoveTo command with only Tag field set to OpenOneQuarter"),
            TestStep("4d", "TH sends MoveTo command with only Tag field set to OpenInHalf"),
            TestStep("4e", "TH sends MoveTo command with only Tag field set to OpenThreeQuarter"),
            TestStep("4f", "TH sends MoveTo command with only Tag field set to OpenOneQuarter"),
            TestStep("4g", "TH sends MoveTo command with only Tag field set to OpenInHalf"),
            TestStep("4h", "TH sends MoveTo command with only Tag field set to OpenThreeQuarter"),
            TestStep("4i", "TH sends MoveTo command with only Tag field set to Pedestrian"),
            TestStep("4j", "TH sends MoveTo command with only Tag field set to Pedestrian"),
            TestStep("4k", "TH sends MoveTo command with only Tag field set to Ventilation"),
            TestStep("4l", "TH sends MoveTo command with only Tag field set to Ventilation"),
            TestStep("4m", "TH sends MoveTo command with only Tag field set to Signature"),
            TestStep("4n", "TH sends MoveTo command with only Tag field set to SequenceNextStep"),
            TestStep("4o", "TH sends MoveTo command with only Tag field set to PedestrianNextStep"),
            TestStep("4p", "TH sends MoveTo command with only Tag field set to PedestrianNextStep"),
            TestStep("4q", "TH sends MoveTo command with only Tag field set to UnknownEnumValue"),
            TestStep("5a", "TH sends MoveTo command with only Latch field set to LatchedAndSecured"),
            TestStep("5b", "TH sends MoveTo command with only Latch field set to LatchedButNotSecured"),
            TestStep("5c", "TH sends MoveTo command with only Latch field set to NotLatched"),
            TestStep("5d", "TH sends MoveTo command with only Latch field set to UnknownEnumValue"),
            TestStep("6a", "TH sends MoveTo command with only Speed field set to Auto"),
            TestStep("6b", "TH sends MoveTo command with only Speed field set to Low"),
            TestStep("6c", "TH sends MoveTo command with only Speed field set to Medium"),
            TestStep("6d", "TH sends MoveTo command with only Speed field set to High"),
            TestStep("6e", "TH sends MoveTo command with only Speed field set to UnknownEnumValue"),
            TestStep("7a", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured"),
            TestStep("7b", "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic"),
            TestStep("7c", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured"),
            TestStep("7d", "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic"),
            TestStep("7e", "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=Automatic"),
            TestStep("7f", "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=UnknownEnumValue"),
            TestStep("7g", "TH sends MoveTo command with fields Tag=Pedestrian, Speed=UnknownEnumValue"),
            TestStep("7h", "TH sends MoveTo command with fields Tag=Ventilation, Latch=UnknownEnumValue"),
            TestStep("7i", "TH sends MoveTo command with fields Tag=UnknownEnumValue, Speed=Automatic"),
            TestStep("7j", "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=UnknownEnumValue"),
            TestStep("7k", "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured, Speed=Automatic"),
        ]
        return steps

    @async_test_body
    async def test_TC_CLOPSTATE_2_2(self):

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

        # STEP 2: TH sends MoveTo command with no field
        self.step(2)
        self.print_step("step number 2", "Send MoveTo command with no field")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo()
        expected_response = Status.InvalidCommand
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 3a
        self.step("3a")
        # if self.is_ci:
        if self.pics_guard(self.check_pics("CLOPSTATE.S.M.ST_SETUPREQUIRED")):
            self.print_step("step number 3a", "Put the device in setup required operational state")
            self.write_to_app_pipe({"Name": "SetSetupRequired", "SetupRequired": True})

            # STEP 3b
            self.step("3b")
            self.print_step("step number 3b", "Read and check the operational state attribute")
            await self.read_operational_state_with_check(0x43)
        else:
            self.skip_step("3b")
            logging.info("Step test 3b skipped. The Dut need to be put in setup required state")

        # STEP 3c
        self.step("3c")
        self.print_step("step number 3c", "Send MoveTo command with tag field equals to CloseInFull")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull)
        expected_response = Status.InvalidInState
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # Reset DUT
        if self.is_ci:
            logging.info("Reset device to default state")
            self.write_to_app_pipe({"Name": "Reset"})

        # STEP 4a
        self.step("4a")
        self.print_step("step number 4a",
                        "TH sends MoveTo command with only Tag field=CloseInFull. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4b
        self.step("4b")
        self.print_step("step number 4a",
                        "TH sends MoveTo command with only Tag field=OpenInFull. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInFull)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4c
        self.step("4c")
        self.print_step("step number 4c",
                        "TH sends MoveTo command with only Tag field=OpenOneQuarter. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenOneQuarter)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4d
        self.step("4d")
        self.print_step("step number 4d",
                        "TH sends MoveTo command with only Tag field=OpenInHalf. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4e
        self.step("4e")
        self.print_step("step number 4e",
                        "TH sends MoveTo command with only Tag field=OpenThreeQuarter. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenThreeQuarter)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F22"):
            # STEP 4f
            self.step("4f")
            self.print_step("step number 4f",
                            "TH sends MoveTo command with only Tag field=Pedestrian. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenOneQuarter)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F22"):
            # STEP 4g
            self.step("4g")
            self.print_step("step number 4g",
                            "TH sends MoveTo command with only Tag field=OpenInHalf. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F22"):
            # STEP 4h
            self.step("4h")
            self.print_step("step number 4h",
                            "TH sends MoveTo command with only Tag field=OpenThreeQuarter. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenThreeQuarter)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4i
        self.step("4i")
        self.print_step("step number 4i",
                        "TH sends MoveTo command with only Tag field=Pedestrian. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrian)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F25"):
            # STEP 4j
            self.step("4j")
            self.print_step("step number 4j",
                            "TH sends MoveTo command with only Tag field=Pedestrian. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrian)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4k
        self.step("4k")
        self.print_step("step number 4k",
                        "TH sends MoveTo command with only Tag field=Ventilation. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kVentilation)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F24"):
            # STEP 4l
            self.step("4l")
            self.print_step("step number 4l",
                            "TH sends MoveTo command with only Tag field=Ventilation. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kVentilation)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4m
        self.step("4m")
        self.print_step("step number 4m",
                        "TH sends MoveTo command with only Tag field=Signature. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kSignature)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4n
        self.step("4n")
        self.print_step("step number 4n",
                        "TH sends MoveTo command with only Tag field=SequenceNextStep. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kSequenceNextStep)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4o
        self.step("4o")
        self.print_step("step number 4o",
                        "TH sends MoveTo command with only Tag field=PedestrianNextStep. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrianNextStep)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        if not self.check_pics("CLOPSTATE.S.F25"):
            # STEP 4p
            self.step("4p")
            self.print_step("step number 4p",
                            "TH sends MoveTo command with only Tag field=PedestrianNextStep. Verify DUT responds with status NOT_FOUND(0x86)")
            cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
                tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrianNextStep)
            expected_response = Status.NotFound
            await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 4q
        self.step("4q")
        self.print_step("step number 4q",
                        "TH sends MoveTo command with only Tag field set to UnknownEnumValue. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # CHECK LATCH FIELD
        # STEP 5a
        self.step("5a")
        self.print_step("step number 5a",
                        "TH sends MoveTo command with only the Latch field set to LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5b
        self.step("5b")
        self.print_step("step number 5b",
                        "TH sends MoveTo command with only the Latch field set to LatchedButNotSecured. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedButNotSecured)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5c
        self.step("5c")
        self.print_step("step number 5c",
                        "TH sends MoveTo command with only the Latch field set to NotLatched. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kNotLatched)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 5d
        self.step("5d")
        self.print_step("step number 5d",
                        "TH sends MoveTo command with only the Latch field set to UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # CHECK SPEED FIELD
        # STEP 6a
        self.step("6a")
        self.print_step("step number 6a",
                        "TH sends MoveTo command with only the Speed field set to Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6b
        self.step("6b")
        self.print_step("step number 6b",
                        "TH sends MoveTo command with only the Speed field set to Low. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kLow)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6c
        self.step("6c")
        self.print_step("step number 6c",
                        "TH sends MoveTo command with only the Speed field set to Medium. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kMedium)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6d
        self.step("6d")
        self.print_step("step number 6d",
                        "TH sends MoveTo command with only the Speed field set to High. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kHigh)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 6e
        self.step("6e")
        self.print_step("step number 6e",
                        "TH sends MoveTo command with only the Speed field set to UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # CHECK MULTIPLE FIELDS
        # STEP 7a
        self.step("7a")
        self.print_step("step number 7a",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7b
        self.step("7b")
        self.print_step("step number 7b",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7c
        self.step("7c")
        self.print_step("step number 7c",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=LatchedAndSecured. Verify DUT responds with status NOT_FOUND(0x8b)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.NotFound
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7d
        self.step("7d")
        self.print_step("step number 7c",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Speed=Automatic. Verify DUT responds with status NOT_FOUND(0x8b)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.NotFound
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7e
        self.step("7e")
        self.print_step("step number 7e",
                        "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=Automatic. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7f
        self.step("7f")
        self.print_step("step number 7f",
                        "TH sends MoveTo command with fields Tag=OpenInHalf, Latch=UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kOpenInHalf,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7g
        self.step("7g")
        self.print_step("step number 7g",
                        "TH sends MoveTo command with fields Tag=Pedestrian, Speed=UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kPedestrian,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7h
        self.step("7h")
        self.print_step("step number 7h",
                        "TH sends MoveTo command with fields Tag=Ventilation, Latch=UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kVentilation,
            speed=Clusters.ClosureOperationalState.Enums.LatchingEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7i
        self.step("7i")
        self.print_step("step number 7i",
                        "TH sends MoveTo command with fields Tag=UnknownEnumValue, Speed=Automatic. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kUnknownEnumValue,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kAutomatic)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7j
        self.step("7j")
        self.print_step("step number 7j",
                        "TH sends MoveTo command with fields Latch=LatchedAndSecured, Speed=UnknownEnumValue. Verify DUT responds with status CONSTRAINT_ERROR(0x87)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured,
            speed=Clusters.Globals.Enums.ThreeLevelAutoEnum.kUnknownEnumValue)
        expected_response = Status.ConstraintError
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)

        # STEP 7k
        self.step("7k")
        self.print_step("step number 7k",
                        "TH sends MoveTo command with fields Tag=CloseInFull, Latch=LatchedAndSecured. Verify DUT responds with status SUCCESS(0x00)")
        cmd = Clusters.Objects.ClosureOperationalState.Commands.MoveTo(
            tag=Clusters.ClosureOperationalState.Enums.TagEnum.kCloseInFull,
            latch=Clusters.ClosureOperationalState.Enums.LatchingEnum.kLatchedAndSecured)
        expected_response = Status.Success
        await self.common_ops.send_cmd_expect_response(self.endpoint, cmd, expected_response)


if __name__ == "__main__":
    default_matter_test_main()