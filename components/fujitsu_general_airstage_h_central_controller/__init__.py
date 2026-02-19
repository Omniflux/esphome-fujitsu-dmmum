import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    tzsp,
    uart,
)

from esphome.const import CONF_ID

CODEOWNERS = ["@Omniflux"]
DEPENDENCIES = ["tzsp", "uart"]
AUTO_LOAD = ["tzsp"]

CONF_CENTRAL_CONTROLLER_ID = "fujitsu_general_airstage_h_central_controller"
CONF_TRANSMIT = "transmit"

fujitsu_general_airstage_h_central_controller_ns = cg.esphome_ns.namespace("fujitsu_general_airstage_h_central_controller")
FujitsuGeneralAirStageHCentralController = fujitsu_general_airstage_h_central_controller_ns.class_("FujitsuGeneralAirStageHCentralController", cg.Component, uart.UARTDevice)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    CONF_CENTRAL_CONTROLLER_ID,
    require_tx=True,
    require_rx=True,
    baud_rate=500,
    data_bits=8,
    parity="EVEN",
    stop_bits=1,
)

CONFIG_SCHEMA = cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FujitsuGeneralAirStageHCentralController),
            cv.Optional(CONF_TRANSMIT, default=True): cv.boolean,
        },
    ).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA).extend(tzsp.TZSP_SENDER_SCHEMA)

CENTRAL_CONTROLLER_CHILD_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_CENTRAL_CONTROLLER_ID): cv.use_id(FujitsuGeneralAirStageHCentralController),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[uart.CONF_UART_ID]))
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await tzsp.register_tzsp_sender(var, config)

    uart.request_wake_loop_on_rx()

    cg.add(var.set_transmit(config[CONF_TRANSMIT]))
