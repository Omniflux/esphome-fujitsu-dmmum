#include "Controller.h"

#include <algorithm>

//#include <esp_log.h>
// Log through esphome instead of standard esp logging
#include <esphome/core/log.h>
using esphome::esp_log_printf_;

namespace fujitsu_general::airstage::h::central_controller {

static const auto TAG = "fujitsu_general::airstage::h::central_controller";

constexpr auto bits_per_symbol() {
    return
        1 + // Start bit
        5 + UARTConfig.data_bits + // Data bits (enum starts at 5 bits = 0)
        (UARTConfig.parity != UART_PARITY_DISABLE) + // Parity bit
        (UARTConfig.stop_bits == UART_STOP_BITS_1 ? 1 : UARTConfig.stop_bits == UART_STOP_BITS_1_5 ? 1.5 : 2) // Stop bits (enum is 1,2,3)
    ;
}
constexpr auto milliseconds_per_second = 1000;
constexpr auto InterPacketDelay = milliseconds_per_second * UARTInterPacketSymbolSpacing * Packet::FrameSize * bits_per_symbol() / UARTConfig.baud_rate;
constexpr auto TokenTimeout = 1.5 * InterPacketDelay;

bool Controller::start() {
    int err;
    auto uart_config = UARTConfig;
    constexpr int intr_alloc_flags = 0;
    constexpr uint32_t queue_size = 20; // ?
    constexpr uint32_t stack_depth = 4096; // ?
    constexpr UBaseType_t task_priority = 12; // ?

    // User should have called uart_set_pin before this point if necessary

    if (this->uart_event_queue == nullptr && uart_is_driver_installed(this->uart_num)) {
        err = uart_driver_delete(this->uart_num);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete UART driver: %s", esp_err_to_name(err));
            return false;
        }
    }

    if (!uart_is_driver_installed(this->uart_num)) {
        const auto buffer_size = UART_HW_FIFO_LEN(this->uart_num) * 2;
        err = uart_driver_install(this->uart_num, buffer_size, buffer_size, queue_size, &this->uart_event_queue, intr_alloc_flags);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
            return false;
        }
    }

    err = uart_param_config(this->uart_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_set_mode(this->uart_num, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART mode: %s", esp_err_to_name(err));
        return false;
    }

    // Default timeout (time to transmit 10 characters at 500bps) is too long
    err = uart_set_rx_timeout(this->uart_num, UARTInterPacketSymbolSpacing);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART RX timeout: %s", esp_err_to_name(err));
        return false;
    }

    xTaskCreate([](void* o){ static_cast<Controller*>(o)->uart_event_task(); }, "UART_Event", stack_depth, this, task_priority, NULL);

    return true;
}

void Controller::uart_event_task() {
    uart_event_t event;

    for(;;) {
        // Transmit updates
        for (const auto& [unit, config] : this->changed_configuration)
        {
            ESP_LOGD(TAG, "Update for unit: %u", unit);
            Packet packet;
            packet.SourceType = AddressTypeEnum::Controller;
            packet.TokenDestinationType = AddressTypeEnum::Controller;
            packet.Config = config;
            packet.Config.IndoorUnitBitfield = 1 << unit;

            if (packet.Config.Setpoint == 0)
                packet.Config.Setpoint = this->current_configuration[unit].Setpoint;

            if (packet.Config.FanSpeed == FanSpeedEnum::NoChange)
                packet.Config.FanSpeed = this->current_configuration[unit].FanSpeed;

            if (packet.Config.Mode == ModeEnum::NoChange)
                packet.Config.Mode = this->current_configuration[unit].Mode;

            if (packet.Config.Controller.Enabled == TriStateEnum::NoChange)
                packet.Config.Controller.Enabled = boolToTriState(this->current_configuration[unit].OutdoorUnit.Enabled);

            if (packet.Config.Controller.Economy == TriStateEnum::NoChange)
                packet.Config.Controller.Economy = boolToTriState(this->current_configuration[unit].OutdoorUnit.Economy);

            if (packet.Config.Controller.MinHeat == TriStateEnum::NoChange)
                packet.Config.Controller.MinHeat = boolToTriState(this->current_configuration[unit].OutdoorUnit.MinHeat);

            if (packet.Config.Controller.LowNoise == TriStateEnum::NoChange)
                packet.Config.Controller.LowNoise = boolToTriState(this->current_configuration[unit].OutdoorUnit.LowNoise);

            if (packet.Config.Controller.RCProhibit == TriStateEnum::NoChange)
                packet.Config.Controller.RCProhibit = boolToTriState(this->current_configuration[unit].OutdoorUnit.RCProhibit);

            Packet::Buffer buffer = packet.to_buffer();
            this->uart_write_bytes(buffer.data(), buffer.size());

            // ODU will only process first packet if they are not sent as separate frames
            vTaskDelay(pdMS_TO_TICKS(InterPacketDelay));
        }
        this->changed_configuration.clear();

        // Transmit query
        Packet packet;
        packet.SourceType = AddressTypeEnum::Controller;
        packet.TokenDestinationType = AddressTypeEnum::OutdoorUnit;
        Packet::Buffer buffer = packet.to_buffer();
        this->uart_write_bytes(buffer.data(), buffer.size());

        // Listen for incoming packets
        for(AddressTypeEnum TokenHolder = AddressTypeEnum::OutdoorUnit; TokenHolder != AddressTypeEnum::Controller;) {
            if (xQueueReceive(this->uart_event_queue, &event, pdMS_TO_TICKS(TokenTimeout))) {
                switch(event.type) {
                    [[likely]] case UART_DATA:
                        size_t buffer_len;

                        // Discard partial frame
                        uart_get_buffered_data_len(this->uart_num, &buffer_len);
                        if (auto discard = buffer_len % buffer.size()) {
                            this->uart_read_bytes(buffer.data(), discard);
                            ESP_LOGD(TAG, "Discarded %d bytes", discard);
                        }

                        // For each frame
                        for (auto i = 0; i < event.size / buffer.size(); i++) {
                            this->uart_read_bytes(buffer.data(), buffer.size());

                            // Parse buffer
                            Packet rx_packet(buffer);
                            this->process_packet(rx_packet);
                            TokenHolder = rx_packet.TokenDestinationType;
                        }

                        break;

                    case UART_BREAK:
                        // TODO Why rx break after tx?
                        ESP_LOGD(TAG, "UART break!");
                        break;

                    case UART_BUFFER_FULL:
                        // Something went wrong - don't try to catch up,
                        // just discard all pending data and start over
                        ESP_LOGD(TAG, "UART ring buffer full!");
                        uart_flush_input(this->uart_num);
                        xQueueReset(this->uart_event_queue);
                        break;

                    case UART_FIFO_OVF:
                        // Something went wrong - don't try to catch up,
                        // just discard all pending data and start over
                        ESP_LOGD(TAG, "UART FIFO Overflow!");
                        uart_flush_input(this->uart_num);
                        xQueueReset(this->uart_event_queue);
                        break;

                    case UART_PARITY_ERR:
                        // Don't do anything, just log for debugging
                        ESP_LOGD(TAG, "UART parity error");
                        break;

                    case UART_FRAME_ERR:
                        // Don't do anything, just log for debugging
                        ESP_LOGD(TAG, "UART frame error");
                        break;

                    default:
                        // Don't do anything, just log for debugging
                        ESP_LOGD(TAG, "Unhandled UART event type: %d", event.type);
                        break;
                }
            }
            else
                // Token timed out
                break;
        }  
    }
}

void Controller::uart_read_bytes(uint8_t *buf, size_t length) const {
    if (this->callbacks.ReadBytes)
        callbacks.ReadBytes(buf, length);
    else
        ::uart_read_bytes(this->uart_num, buf, length, portMAX_DELAY);
}

void Controller::uart_write_bytes(const uint8_t *buf, size_t length) const {
    if (this->callbacks.WriteBytes)
        callbacks.WriteBytes(buf, length);
    else
        ::uart_write_bytes(this->uart_num, buf, length);
}

bool Controller::can_control_unit(uint8_t unit, bool ignore_lock) const {
    return this->current_configuration.contains(unit) &&
        (ignore_lock || !this->current_configuration.at(unit).OutdoorUnit.OpRestricted);
}

void Controller::process_packet(const Packet& packet) {
    if (packet.SourceType == AddressTypeEnum::OutdoorUnit) {
        // Setpoint of 0 indicates end of list, not state of an indoor unit
        if (packet.Config.Setpoint) {
            const auto unit = packet.Config.OutdoorUnit.IndoorUnit;

            // Configuration has changed
            if (this->current_configuration[unit] != packet.Config) {
                this->current_configuration[unit] = packet.Config;

                // ODU mode change in progress
                if (this->odu_mode_change_units.contains(unit) && this->odu_mode_change_units.at(unit) == ODUModeChangeStatus::Pending && !packet.Config.OutdoorUnit.Enabled) {
                    this->odu_mode_change_units[unit] = ODUModeChangeStatus::Complete;

                    // Switching modes is complete
                    if (std::ranges::all_of(std::views::values(this->odu_mode_change_units), [](const auto& s) { return s == ODUModeChangeStatus::Complete; })) {
                        for (const auto unit : std::views::keys(this->odu_mode_change_units))
                            this->set_enabled(unit, true, true);

                        this->odu_mode_change_units.clear();
                    }
                }

                if (this->callbacks.Config)
                    this->callbacks.Config(packet.Config);
            }
        }
    }
}

// Heat mode is incompatible with all other modes at the ODU.
// Provide a method of switching the ODU mode if necessary.
// Will not work if mode is locked out via priority operation function (Fujitsu Service Tip #34)
// or if all units needing currently unavailable mode are set to Auto
void Controller::odu_mode_change() {
    if (std::ranges::any_of(std::views::values(this->current_configuration), [](const auto& c) { return c.OutdoorUnit.IncompatibleMode; }))
        for (const auto unit : this->current_configuration | std::ranges::views::filter([](const auto& c) { return c.second.OutdoorUnit.Enabled && !c.second.OutdoorUnit.IncompatibleMode; }) | std::views::keys) {
            this->odu_mode_change_units[unit] = ODUModeChangeStatus::Pending;
            this->set_enabled(unit, false, true);
    }
}

void Controller::set_low_noise(bool low_noise) {
    TriStateEnum ln = boolToTriState(low_noise);

    for (auto const& unit : this->get_known_units())
        this->changed_configuration[unit].Controller.LowNoise = ln;
}

bool Controller::set_rc_prohibit(uint8_t unit, bool rc_prohibit) {
    if (!this->current_configuration.contains(unit))
        return false;

    this->changed_configuration[unit].Controller.RCProhibit = boolToTriState(rc_prohibit);
    return true;
}

bool Controller::set_enabled(uint8_t unit, bool enabled, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    this->changed_configuration[unit].Controller.Enabled = boolToTriState(enabled);
    return true;
}

bool Controller::set_setpoint(uint8_t unit, uint8_t temperature, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    if (temperature < MinSetpoint || temperature > MaxSetpoint)
        return false;

    this->changed_configuration[unit].Setpoint = temperature;
    return true;
}

bool Controller::set_mode(uint8_t unit, ModeEnum mode, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    this->changed_configuration[unit].Mode = mode;
    return true;
}

bool Controller::set_fan_speed(uint8_t unit, FanSpeedEnum fan_speed, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    this->changed_configuration[unit].FanSpeed = fan_speed;
    return true;
}

bool Controller::set_economy(uint8_t unit, bool economy, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    this->changed_configuration[unit].Controller.Economy = boolToTriState(economy);
    return true;
}

bool Controller::set_min_heat(uint8_t unit, bool min_heat, bool ignore_lock) {
    if (!this->can_control_unit(unit, ignore_lock))
        return false;

    this->changed_configuration[unit].Controller.MinHeat = boolToTriState(min_heat);

    if (min_heat)
        this->changed_configuration[unit].Controller.Enabled = boolToTriState(false);

    return true;
}

}
