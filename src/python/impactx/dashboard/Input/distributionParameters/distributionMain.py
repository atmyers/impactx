"""
This file is part of ImpactX

Copyright 2024 ImpactX contributors
Authors: Parthib Roy, Axel Huebl
License: BSD-3-Clause-LBNL
"""

import inspect

from distribution_input_helpers import twiss

from impactx import distribution

from .. import TrameFunctions, generalFunctions, setup_server, vuetify
from .distributionFunctions import DistributionFunctions

server, state, ctrl = setup_server()

# -----------------------------------------------------------------------------
# Helpful
# -----------------------------------------------------------------------------

DISTRIBUTION_MODULE_NAME = distribution
DISTRIBUTION_LIST = generalFunctions.select_classes(DISTRIBUTION_MODULE_NAME)
DISTRIBUTION_PARAMETERS_AND_DEFAULTS = generalFunctions.class_parameters_with_defaults(
    DISTRIBUTION_MODULE_NAME
)

state.selected_distribution_parameters = []
state.distribution_type_disable = False

# -----------------------------------------------------------------------------
# Main Functions
# -----------------------------------------------------------------------------


def populate_distribution_parameters():
    """
    Populates distribution parameters based on the selected distribution.
    :param selected_distribution (str): The name of the selected distribution
    whose parameters need to be populated.
    """

    if state.distribution_type == "Twiss":
        sig = inspect.signature(twiss)
        state.selected_distribution_parameters = [
            {
                "parameter_name": param.name,
                "parameter_default_value": param.default
                if param.default != param.empty
                else None,
                "parameter_type": "float",  # Hardcoding Twiss to 'float' type.
                "parameter_error_message": generalFunctions.validate_against(
                    param.default if param.default != param.empty else None, "float"
                ),
                "parameter_units": generalFunctions.get_default(param.name, "units")
                if "beta" in param.name or "emitt" in param.name
                else "",
                "parameter_step": generalFunctions.get_default(param.name, "steps"),
            }
            for param in sig.parameters.values()
        ]

    else:  # when type == 'Quadratic Form'
        selected_distribution_parameters = DISTRIBUTION_PARAMETERS_AND_DEFAULTS.get(
            state.distribution, []
        )

        state.selected_distribution_parameters = [
            {
                "parameter_name": parameter[0],
                "parameter_default_value": parameter[1],
                "parameter_type": parameter[2],
                "parameter_error_message": generalFunctions.validate_against(
                    parameter[1], parameter[2]
                ),
                "parameter_units": "m"
                if "beta" in parameter[0] or "emitt" in parameter[0]
                else "",
                "parameter_step": generalFunctions.get_default(parameter[0], "steps"),
            }
            for parameter in selected_distribution_parameters
        ]

    generalFunctions.update_simulation_validation_status()
    return state.selected_distribution_parameters


# -----------------------------------------------------------------------------
# Write to file functions
# -----------------------------------------------------------------------------


def distribution_parameters():
    """
    :return: An instance of the selected distribution class,
    initialized with the appropriate parameters provided by the user.
    """

    distribution_name = state.distribution
    parameters = DistributionFunctions.convert_distribution_parameters_to_valid_type()

    if state.distribution_type == "Twiss":
        twiss_params = twiss(**parameters)
        distr = getattr(distribution, distribution_name)(**twiss_params)
    else:
        distr = getattr(distribution, distribution_name)(**parameters)

    return distr


# -----------------------------------------------------------------------------
# Callbacks
# -----------------------------------------------------------------------------


@state.change("distribution")
def on_distribution_name_change(distribution, **kwargs):
    if distribution == "Thermal":
        state.distribution_type = "Quadratic Form"
        state.distribution_type_disable = True
        state.dirty("distribution_type")
    else:
        state.distribution_type_disable = False
    populate_distribution_parameters()


@state.change("distribution_type")
def on_distribution_type_change(**kwargs):
    populate_distribution_parameters()


@ctrl.add("updateDistributionParameters")
def on_distribution_parameter_change(parameter_name, parameter_value, parameter_type):
    parameter_value, input_type = generalFunctions.determine_input_type(parameter_value)
    error_message = generalFunctions.validate_against(parameter_value, parameter_type)

    for param in state.selected_distribution_parameters:
        if param["parameter_name"] == parameter_name:
            param["parameter_default_value"] = parameter_value
            param["parameter_error_message"] = error_message

    generalFunctions.update_simulation_validation_status()
    state.dirty("selected_distribution_parameters")


# -----------------------------------------------------------------------------
# Content
# -----------------------------------------------------------------------------


class DistributionParameters:
    """
    User-Input section for beam distribution.
    """

    @staticmethod
    def card():
        """
        Creates UI content for beam distribution.
        """

        with vuetify.VCard(style="width: 340px; height: 300px"):
            TrameFunctions.input_section_header("Distribution Parameters")
            with vuetify.VCardText():
                with vuetify.VRow():
                    with vuetify.VCol(cols=6):
                        TrameFunctions.select(
                            label="Select Distribution",
                            v_model_name="distribution",
                            items=(DISTRIBUTION_LIST,),
                        )
                    with vuetify.VCol(cols=6):
                        TrameFunctions.select(
                            label="Type",
                            v_model_name="distribution_type",
                            disabled=("distribution_type_disable",),
                        )
                with vuetify.VRow(classes="my-2"):
                    for i in range(3):
                        with vuetify.VCol(cols=4, classes="py-0"):
                            with vuetify.VRow(
                                v_for="(parameter, index) in selected_distribution_parameters",
                                v_if=f"index % 3 == {i}",
                            ):
                                with vuetify.VCol(classes="py-1"):
                                    vuetify.VTextField(
                                        label=("parameter.parameter_name",),
                                        v_model=("parameter.parameter_default_value",),
                                        suffix=("parameter.parameter_units",),
                                        change=(
                                            ctrl.updateDistributionParameters,
                                            "[parameter.parameter_name, $event, parameter.parameter_type]",
                                        ),
                                        error_messages=(
                                            "parameter.parameter_error_message",
                                        ),
                                        type="number",
                                        step=("parameter.parameter_step",),
                                        __properties=["step"],
                                        dense=True,
                                    )
