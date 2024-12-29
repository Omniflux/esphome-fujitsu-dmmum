#pragma once

#include <map>

#include <esphome/core/component.h>
#include <esphome/components/uart/uart.h>
#include <esphome/components/uart/uart_component_esp_idf.h>

#include <esphome/components/tzsp/tzsp.h>

#include "esphome-custom-switch.h"
#include "Controller.h"

namespace esphome::fujitsu_general_airstage_h_central_controller {

class FujitsuGeneralAirStageHCentralController : public Component, public uart::UARTDevice, public tzsp::TZSPSender {
    friend class FujitsuGeneralAirStageHIndoorUnit;

    public:
        CustomSwitch* low_noise_switch = new CustomSwitch([this](bool state) { this->controller->set_low_noise(state); return state; });

        FujitsuGeneralAirStageHCentralController(uart::IDFUARTComponent *parent) : uart::UARTDevice(parent) {}

        void setup() override;
        void dump_config() override;
        float get_setup_priority() const override { return esphome::setup_priority::DATA; }

        void set_config_callback(uint8_t unit, const fujitsu_general::airstage::h::central_controller::Controller::ConfigCallback& config_callback) { config_callbacks[unit] = config_callback; };

    private:
        fujitsu_general::airstage::h::central_controller::Controller* controller;
        std::map<uint8_t, fujitsu_general::airstage::h::central_controller::Controller::ConfigCallback> config_callbacks;

        void log_buffer(const char* dir, const uint8_t* buf, size_t length) const;

        void update_from_device(const fujitsu_general::airstage::h::central_controller::Config& data);

        static constexpr uint8_t uart_data_bits_to_uart_config_data_bits(uart_word_length_t bits) const noexcept;
        static constexpr uint8_t uart_stop_bits_to_uart_config_stop_bits(uart_stop_bits_t bits) const noexcept;
        static constexpr uart::UARTParityOptions uart_parity_to_uart_config_parity(uart_parity_t parity) const noexcept;
};

}