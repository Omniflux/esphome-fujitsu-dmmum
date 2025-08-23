import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    climate,
    sensor,
)

from esphome.const import (
    CONF_SENSOR,
    CONF_HUMIDITY_SENSOR,
)

from .. import (
    CENTRAL_CONTROLLER_CHILD_SCHEMA,
    CONF_CENTRAL_CONTROLLER_ID,
    fujitsu_general_airstage_h_central_controller_ns,
)

CODEOWNERS = ["@Omniflux"]
AUTO_LOAD = [CONF_CENTRAL_CONTROLLER_ID]

CONF_INDOOR_UNIT_ID = "indoor_unit_id"
CONF_INDOOR_UNIT_NUMBER = "indoor_unit"
CONF_IGNORE_LOCK = "ignore_lock"

FujitsuGeneralAirStageHIndoorUnit = fujitsu_general_airstage_h_central_controller_ns.class_("FujitsuGeneralAirStageHIndoorUnit", cg.Component, climate.Climate)

CONFIG_SCHEMA = CENTRAL_CONTROLLER_CHILD_SCHEMA.extend(
    climate.climate_schema(FujitsuGeneralAirStageHIndoorUnit).extend(
        {
            cv.Required(CONF_INDOOR_UNIT_NUMBER): cv.int_range(1,6),
            cv.Optional(CONF_IGNORE_LOCK, default=False): cv.boolean,
            cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        }
    )
)

INDOOR_UNIT_CHILD_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_INDOOR_UNIT_ID): cv.use_id(FujitsuGeneralAirStageHIndoorUnit),
    }
)

async def to_code(config):
    controller = await cg.get_variable(config[CONF_CENTRAL_CONTROLLER_ID])

    var = await climate.new_climate(config, controller, config[CONF_INDOOR_UNIT_NUMBER])
    await cg.register_component(var, config)

    if CONF_IGNORE_LOCK in config:
        cg.add(var.set_ignore_lock(config[CONF_IGNORE_LOCK]))

    if CONF_SENSOR in config:
        cg.add(var.set_temperature_sensor(await cg.get_variable(config[CONF_SENSOR])))

    if CONF_HUMIDITY_SENSOR in config:
        cg.add(var.set_humidity_sensor(await cg.get_variable(config[CONF_HUMIDITY_SENSOR])))
