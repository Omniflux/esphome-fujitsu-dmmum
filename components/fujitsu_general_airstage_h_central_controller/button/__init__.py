import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import button

from esphome.const import (
    CONF_ID,
    ENTITY_CATEGORY_CONFIG,
)

from .. import (
    CENTRAL_CONTROLLER_CHILD_SCHEMA,
    CONF_CENTRAL_CONTROLLER_ID,
)

CODEOWNERS = ["@Omniflux"]

CONF_ODU_MODE_CHANGE = "odu_mode_change"

custom_ns = cg.esphome_ns.namespace("custom")
CustomButton = custom_ns.class_("CustomButton", button.Button, cg.Component)

CONFIG_SCHEMA = CENTRAL_CONTROLLER_CHILD_SCHEMA.extend(
    {
        cv.Optional(CONF_ODU_MODE_CHANGE): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
)

async def to_code(config):
    controller = await cg.get_variable(config[CONF_CENTRAL_CONTROLLER_ID])

    var = cg.Pvariable(config[CONF_ODU_MODE_CHANGE][CONF_ID], controller.odu_mode_change_button)
    await button.register_button(var, config[CONF_ODU_MODE_CHANGE])
