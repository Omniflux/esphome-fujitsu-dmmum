import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    tzsp,
    uart,
    switch
)

from esphome.const import (
    CONF_ID,
    CONF_NAME,
    ENTITY_CATEGORY_CONFIG
)

CODEOWNERS = ["@Omniflux"]
DEPENDENCIES = ["tzsp", "uart"]
AUTO_LOAD = ["switch", "tzsp"]

CONF_CENTRAL_CONTROLLER_ID = "fujitsu_general_airstage_h_central_controller"
CONF_LOW_NOISE = "low_noise"

fujitsu_general_airstage_h_central_controller_ns = cg.esphome_ns.namespace("fujitsu_general_airstage_h_central_controller")
CustomSwitch = fujitsu_general_airstage_h_central_controller_ns.class_("CustomSwitch", cg.Component, switch.Switch)
FujitsuGeneralAirStageHCentralController = fujitsu_general_airstage_h_central_controller_ns.class_("FujitsuGeneralAirStageHCentralController", cg.Component, uart.UARTDevice)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    CONF_CENTRAL_CONTROLLER_ID,
    require_tx=True,
    require_rx=True,
    baud_rate=500,
    data_bits=8,
    parity="EVEN",
    stop_bits=1
)

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FujitsuGeneralAirStageHCentralController),
        cv.Optional(CONF_LOW_NOISE, default={CONF_NAME: "Low Noise Mode"}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_OFF"
        )
    }
).extend(cv.COMPONENT_SCHEMA).extend(tzsp.TZSP_SENDER_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[uart.CONF_UART_ID]))
    await cg.register_component(var, config)
    await tzsp.register_tzsp_sender(var, config)
    await uart.register_uart_device(var, config)

    varx = cg.Pvariable(config[CONF_LOW_NOISE][CONF_ID], var.low_noise_switch)
    await switch.register_switch(varx, config[CONF_LOW_NOISE])