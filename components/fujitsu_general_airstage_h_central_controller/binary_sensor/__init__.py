import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import binary_sensor

from esphome.const import (
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

from ..climate import (
    INDOOR_UNIT_CHILD_SCHEMA,
    CONF_INDOOR_UNIT_ID,
)

CODEOWNERS = ["@Omniflux"]

CONF_INCOMPATIBLE_MODE = "incompatible_mode"
CONF_ERROR_STATE = "error_state"

BinarySensor = cg.esphome_ns.class_("BinarySensor", binary_sensor.BinarySensor, cg.Component)

CONFIG_SCHEMA = INDOOR_UNIT_CHILD_SCHEMA.extend(
    {
        cv.Optional(CONF_INCOMPATIBLE_MODE): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM,
        ),
        cv.Optional(CONF_ERROR_STATE): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM,
        ),
    }
)

async def to_code(config):
    controller = await cg.get_variable(config[CONF_INDOOR_UNIT_ID])

    if CONF_INCOMPATIBLE_MODE in config:
        var = await binary_sensor.new_binary_sensor(config[CONF_INCOMPATIBLE_MODE])
        cg.add(controller.set_incompatible_mode_binary_sensor(var))

    if CONF_ERROR_STATE in config:
        var = await binary_sensor.new_binary_sensor(config[CONF_ERROR_STATE])
        cg.add(controller.set_error_binary_sensor(var))
