import logging
import chip.clusters as Clusters
from chip.interaction_model import InteractionModelError, Status
from mobly import asserts


class ClosureCommonOperations:
    def __init__(self, test_instance):
        self.test_instance = test_instance

    # Converts an operational state enum to a string representation
    @staticmethod
    def state_enum_to_text(state_enum):
        if state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kStopped:
            return "Stopped(0x00)"
        elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kRunning:
            return "Running(0x01)"
        elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kPaused:
            return "Paused(0x02)"
        elif state_enum == Clusters.OperationalState.Enums.OperationalStateEnum.kError:
            return "Error(0x03)"
        elif state_enum == Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kProtected:
            return "Protected(0x41)"
        elif state_enum == Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kDisengaded:
            return "Disengaged(0x42)"
        elif state_enum == Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kSetupRequired:
            return "SetupRequired(0x43)"
        elif state_enum == Clusters.ClosureOperationalState.Enums.OperationalStateEnum.kCalibrating:
            return "Calibrating(0x64)"
        else:
            return "UnknownEnumValue"

    # Reads the operational state attribute and checks if it matches the expected state
    async def read_operational_state_with_check(self, expected_state):
        operational_state = await self.test_instance.read_single_attribute_check_success(
            endpoint=self.test_instance.endpoint,
            cluster=Clusters.Objects.ClosureOperationalState,
            attribute=Clusters.ClosureOperationalState.Attributes.OperationalState
        )
        logging.info(f"OperationalState: {operational_state}")
        asserts.assert_equal(operational_state, expected_state,
                             f"OperationalState({operational_state}) should be {self.state_enum_to_text(expected_state)}")

    # Sends a command and checks if the response matches the expected response status
    async def send_cmd_expect_response(self, endpoint, cmd, expected_response_status, timedRequestTimeoutMs=None):
        try:
            await self.test_instance.send_single_cmd(endpoint=endpoint,
                                                     cmd=cmd,
                                                     timedRequestTimeoutMs=timedRequestTimeoutMs)
        except InteractionModelError as e:
            asserts.assert_equal(e.status,
                                 expected_response_status,
                                 f"Command response ({e.status}) mismatched from expected status {expected_response_status} for {cmd} on {endpoint}")

    # Reads the overall state attribute and checks if it matches the expected instance type
    async def read_overall_state_attr_with_check_of_instance(self):
        attribute_to_check = Clusters.ClosureOperationalState.Attributes.OverallState
        overall_state = await self.test_instance.read_single_attribute(dev_ctrl=self.test_instance.default_controller, node_id=self.test_instance.dut_node_id,
                                                                       endpoint=self.test_instance.endpoint, attribute=attribute_to_check
                                                                       )
        logging.info(f"OverallState: {overall_state}")
        asserts.assert_true(isinstance(overall_state, Clusters.ClosureOperationalState.Structs.OverallStateStruct),
                            f"OverallState Read is not an instance of {attribute_to_check}")
        return overall_state

    # Reads the operational error attribute and checks if it matches the expected error
    async def read_operational_error_with_check(self, expected_error):
        operational_error = await self.test_instance.read_single_attribute_check_success(
            endpoint=self.test_instance.endpoint,
            cluster=Clusters.Objects.ClosureOperationalState,
            attribute=Clusters.ClosureOperationalState.Attributes.OperationalError
        )
        logging.info(f"OperationalError: {operational_error}")
        asserts.assert_equal(operational_error.errorStateID, expected_error,
                             f"OperationalError({operational_error}) should be {expected_error}")
        asserts.assert_true(isinstance(operational_error, Clusters.ClosureOperationalState.Structs.ErrorStateStruct),
                            f"OperationalError Read is not an instance of ErrorStateStruct")
