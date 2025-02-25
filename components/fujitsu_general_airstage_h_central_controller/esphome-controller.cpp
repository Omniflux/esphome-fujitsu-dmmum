#include "esphome-controller.h"

namespace esphome::fujitsu_general_airstage_h_central_controller {

static const auto TAG = "esphome::fujitsu_general_airstage_h_central_controller::controller";

void FujitsuGeneralAirStageHCentralController::setup() {
    this->controller = new fujitsu_general::airstage::h::central_controller::Controller(
        static_cast<uart::IDFUARTComponent*>(this->parent_)->get_hw_serial_number(),
        {
            .Config = [this](const fujitsu_general::airstage::h::central_controller::Config& data){ this->update_from_device(data); },
            .ReadBytes  = [this](uint8_t *buf, size_t length){
                this->read_array(buf, length);
                this->log_buffer("RX", buf, length);
            },
            .WriteBytes = [this](const uint8_t *buf, size_t length){
                this->write_array(buf, length);
                this->log_buffer("TX", buf, length);
            }
        },
        *static_cast<uart::IDFUARTComponent*>(this->parent_)->get_uart_event_queue()
    );

    if (!this->controller->start()) {
        ESP_LOGE(TAG, "Failed to start controller");
        this->mark_failed();
        return;
    }
}

void FujitsuGeneralAirStageHCentralController::log_buffer(const char* dir, const uint8_t* buf, size_t length) const {
    auto tbuf = std::vector<uint8_t>(buf, buf + length);
    for (auto &b : tbuf)
        b ^= 0xFF;

    this->tzsp_send(tbuf);
    ESP_LOGD(TAG, "%s: %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX", dir, tbuf[0], tbuf[1], tbuf[2], tbuf[3], tbuf[4], tbuf[5], tbuf[6], tbuf[7]);
}

void FujitsuGeneralAirStageHCentralController::dump_config() {
    ESP_LOGCONFIG(TAG, "FujitsuGeneralAirStageHCentralController:");
    for (auto unit : this->controller->get_known_units())
        ESP_LOGCONFIG(TAG, "  Indoor Unit: %u", unit + 1);
    LOG_TZSP("  ", this);

    this->check_uart_settings(
        fujitsu_general::airstage::h::central_controller::UARTConfig.baud_rate,
        this->uart_stop_bits_to_uart_config_stop_bits(fujitsu_general::airstage::h::central_controller::UARTConfig.stop_bits),
        this->uart_parity_to_uart_config_parity(fujitsu_general::airstage::h::central_controller::UARTConfig.parity),
        this->uart_data_bits_to_uart_config_data_bits(fujitsu_general::airstage::h::central_controller::UARTConfig.data_bits)
    );
}

void FujitsuGeneralAirStageHCentralController::update_from_device(const fujitsu_general::airstage::h::central_controller::Config& data) {
    if (data.OutdoorUnit.LowNoise != this->low_noise_switch->state)
        this->low_noise_switch->publish_state(data.OutdoorUnit.LowNoise);

    auto callback = this->config_callbacks.find(data.OutdoorUnit.IndoorUnit);
    if (callback != this->config_callbacks.end())
        callback->second(data);
}

constexpr uint8_t FujitsuGeneralAirStageHCentralController::uart_data_bits_to_uart_config_data_bits(uart_word_length_t bits) {
    switch (bits) {
        case UART_DATA_5_BITS: return 5;
        case UART_DATA_6_BITS: return 6;
        case UART_DATA_7_BITS: return 7;

        // ESPHome UART only supports 5, 6, 7, 8
        default: return 8;
    }
}

constexpr uint8_t FujitsuGeneralAirStageHCentralController::uart_stop_bits_to_uart_config_stop_bits(uart_stop_bits_t bits) {
    switch (bits) {
        case UART_STOP_BITS_1: return 1;

        // ESPHome UART only supports 1 and 2
        default: return 2;
    }
}

constexpr uart::UARTParityOptions FujitsuGeneralAirStageHCentralController::uart_parity_to_uart_config_parity(uart_parity_t parity) {
    switch (parity) {
        case UART_PARITY_EVEN:  return uart::UART_CONFIG_PARITY_EVEN;
        case UART_PARITY_ODD:   return uart::UART_CONFIG_PARITY_ODD;
        default:                return uart::UART_CONFIG_PARITY_NONE;
    }
}

}