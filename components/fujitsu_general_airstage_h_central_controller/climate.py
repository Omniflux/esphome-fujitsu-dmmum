import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    binary_sensor,
    climate,
    sensor,
    switch
)

from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_SENSOR,
    CONF_HUMIDITY_SENSOR,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC
)

from . import (
    CONF_CENTRAL_CONTROLLER_ID,
    fujitsu_general_airstage_h_central_controller_ns,
    FujitsuGeneralAirStageHCentralController,
)

CODEOWNERS = ["@Omniflux"]
AUTO_LOAD = ["binary_sensor", "climate", "sensor", "switch", CONF_CENTRAL_CONTROLLER_ID]

CONF_INDOOR_UNIT = 'indoor_unit'
CONF_IGNORE_LOCK = "ignore_lock"
CONF_INCOMPATIBLE_MODE = "incompatible_mode"
CONF_ERROR_STATE = "error_state"
CONF_MIN_HEAT = "min_heat"
CONF_RC_PROHIBIT = "rc_prohibit"

BinarySensor = cg.esphome_ns.class_("BinarySensor", cg.Component, binary_sensor.BinarySensor)
CustomSwitch = fujitsu_general_airstage_h_central_controller_ns.class_("CustomSwitch", cg.Component, switch.Switch)
FujitsuGeneralAirStageHIndoorUnit = fujitsu_general_airstage_h_central_controller_ns.class_("FujitsuGeneralAirStageHIndoorUnit", cg.Component, climate.Climate)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FujitsuGeneralAirStageHIndoorUnit),
        cv.GenerateID(CONF_CENTRAL_CONTROLLER_ID): cv.use_id(FujitsuGeneralAirStageHCentralController),
        cv.Required(CONF_INDOOR_UNIT): cv.int_range(1,6),
        cv.Optional(CONF_IGNORE_LOCK, default=False): cv.boolean,
        cv.Optional(CONF_INCOMPATIBLE_MODE, default={CONF_NAME: "Incompatible Mode"}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ),
        cv.Optional(CONF_ERROR_STATE, default={CONF_NAME: "Error"}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ),
        cv.Optional(CONF_MIN_HEAT, default={CONF_NAME: "Min. Heat"}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_RC_PROHIBIT, default={CONF_NAME: "RC Prohibit"}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_OFF"
        ),
        cv.Optional(CONF_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor)
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[CONF_CENTRAL_CONTROLLER_ID]), config[CONF_INDOOR_UNIT])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    cg.add(var.set_ignore_lock(config[CONF_IGNORE_LOCK]))

    config[CONF_INCOMPATIBLE_MODE][CONF_NAME] = config[CONF_NAME] + " " + config[CONF_INCOMPATIBLE_MODE][CONF_NAME]
    varx = cg.Pvariable(config[CONF_INCOMPATIBLE_MODE][CONF_ID], var.incompatible_mode_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_INCOMPATIBLE_MODE])

    config[CONF_ERROR_STATE][CONF_NAME] = config[CONF_NAME] + " " + config[CONF_ERROR_STATE][CONF_NAME]
    varx = cg.Pvariable(config[CONF_ERROR_STATE][CONF_ID], var.error_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_ERROR_STATE])

    config[CONF_MIN_HEAT][CONF_NAME] = config[CONF_NAME] + " " + config[CONF_MIN_HEAT][CONF_NAME]
    varx = cg.Pvariable(config[CONF_MIN_HEAT][CONF_ID], var.min_heat_switch)
    await switch.register_switch(varx, config[CONF_MIN_HEAT])

    config[CONF_RC_PROHIBIT][CONF_NAME] = config[CONF_NAME] + " " + config[CONF_RC_PROHIBIT][CONF_NAME]
    varx = cg.Pvariable(config[CONF_RC_PROHIBIT][CONF_ID], var.rc_prohibit_switch)
    await switch.register_switch(varx, config[CONF_RC_PROHIBIT])

    if CONF_SENSOR in config:
        cg.add(var.set_temperature_sensor(await cg.get_variable(config[CONF_SENSOR])))

    if CONF_HUMIDITY_SENSOR in config:
        cg.add(var.set_humidity_sensor(await cg.get_variable(config[CONF_HUMIDITY_SENSOR])))