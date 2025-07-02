#pragma once

#include <functional>
#include <map>
#include <ranges>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/uart.h>

#include "Packet.h"

namespace fujitsu_general::airstage::h::central_controller {

constexpr uart_config_t UARTConfig = {
        .baud_rate = 500,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
};

constexpr uint8_t UARTInterPacketSymbolSpacing = 2;

// Temperatures are in Celcius
constexpr uint8_t MinSetpoint = 16;
constexpr uint8_t MaxSetpoint = 30;

enum class ODUModeChangeStatus : bool {
    Pending,
    Complete
};

class Controller {
    public:
        using ConfigCallback = std::function<void(const Config&)>;
        using ReadBytesCallback  = std::function<void(uint8_t *data, size_t len)>;
        using WriteBytesCallback = std::function<void(const uint8_t *data, size_t len)>;

        struct Callbacks {
            ConfigCallback Config;
            ReadBytesCallback ReadBytes;
            WriteBytesCallback WriteBytes;
        };

        Controller(uint8_t uart_num, const Callbacks& callbacks, QueueHandle_t uart_event_queue = nullptr)
            : uart_num(uart_num), uart_event_queue(uart_event_queue), callbacks(callbacks) {}

        bool start();

        void odu_mode_change();
        void set_low_noise(bool low_noise);

        bool set_rc_prohibit(uint8_t unit, bool rc_prohibit);
        bool set_enabled(uint8_t unit, bool enabled, bool ignore_lock = false);
        bool set_setpoint(uint8_t unit, uint8_t temperature, bool ignore_lock = false);
        bool set_mode(uint8_t unit, ModeEnum mode, bool ignore_lock = false);
        bool set_fan_speed(uint8_t unit, FanSpeedEnum fan_speed, bool ignore_lock = false);
        bool set_economy(uint8_t unit, bool economy, bool ignore_lock = false);
        bool set_min_heat(uint8_t unit, bool min_heat, bool ignore_lock = false);

        auto get_known_units() const { return std::views::keys(this->current_configuration); }
        bool can_control_unit(uint8_t unit, bool ignore_lock = false) const;

    protected:
        void process_packet(const Packet& packet);

    private:
        uint8_t uart_num;
        QueueHandle_t uart_event_queue;
        Callbacks callbacks;

        std::map<uint8_t, struct Config> current_configuration;
        std::map<uint8_t, struct Config> changed_configuration;
        std::map<uint8_t, ODUModeChangeStatus> odu_mode_change_units;

        [[noreturn]] void uart_event_task();
        void uart_read_bytes(uint8_t *buf, size_t length) const;
        void uart_write_bytes(const uint8_t *buf, size_t length) const;
};

}
