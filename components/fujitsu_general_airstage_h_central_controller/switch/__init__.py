import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import switch

from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)

from .. import (
    CENTRAL_CONTROLLER_CHILD_SCHEMA,
    CONF_CENTRAL_CONTROLLER_ID,
)

from ..climate import (
    INDOOR_UNIT_CHILD_SCHEMA,
    CONF_INDOOR_UNIT_ID,
)

CODEOWNERS = ["@Omniflux"]

CONF_LOW_NOISE = "low_noise"
CONF_MIN_HEAT = "min_heat"
CONF_RC_PROHIBIT = "rc_prohibit"

custom_ns = cg.esphome_ns.namespace("custom")
CustomSwitch = custom_ns.class_("CustomSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Any(
    CENTRAL_CONTROLLER_CHILD_SCHEMA.extend(
        {
            cv.Optional(CONF_LOW_NOISE):switch.switch_schema(
                CustomSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                default_restore_mode="RESTORE_DEFAULT_OFF",
            ),
        }
    ),
    INDOOR_UNIT_CHILD_SCHEMA.extend(
        {
            cv.Optional(CONF_MIN_HEAT): switch.switch_schema(
                CustomSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                default_restore_mode="RESTORE_DEFAULT_ON",
            ),
            cv.Optional(CONF_RC_PROHIBIT): switch.switch_schema(
                CustomSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                default_restore_mode="RESTORE_DEFAULT_OFF",
            ),
        }
    ),
)

async def to_code(config):
    if CONF_CENTRAL_CONTROLLER_ID in config:
        controller = await cg.get_variable(config[CONF_CENTRAL_CONTROLLER_ID])
        var = cg.Pvariable(config[CONF_LOW_NOISE][CONF_ID], controller.low_noise_switch)
        await switch.register_switch(var, config[CONF_LOW_NOISE])

    elif CONF_INDOOR_UNIT_ID in config:
        controller = await cg.get_variable(config[CONF_INDOOR_UNIT_ID])

        var = cg.Pvariable(config[CONF_MIN_HEAT][CONF_ID], controller.min_heat_switch)
        await switch.register_switch(var, config[CONF_MIN_HEAT])

        var = cg.Pvariable(config[CONF_RC_PROHIBIT][CONF_ID], controller.rc_prohibit_switch)
        await switch.register_switch(var, config[CONF_RC_PROHIBIT])
