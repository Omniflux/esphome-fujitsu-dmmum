#pragma once

#include <esphome/core/component.h>
#include <esphome/components/climate/climate.h>

#ifdef USE_BINARY_SENSOR
#include <esphome/components/binary_sensor/binary_sensor.h>
#endif
#ifdef USE_SENSOR
#include <esphome/components/sensor/sensor.h>
#endif

#ifdef USE_SWITCH
#include "esphome-custom-switch.h"
#endif

#include "esphome-controller.h"

namespace esphome::fujitsu_general_airstage_h_central_controller {

class FujitsuGeneralAirStageHIndoorUnit : public Component, public climate::Climate {
#ifdef USE_SENSOR
    SUB_SENSOR(humidity);
    SUB_SENSOR(temperature);
#endif
#ifdef USE_BINARY_SENSOR
    SUB_BINARY_SENSOR(incompatible_mode);
    SUB_BINARY_SENSOR(error);
#endif

    public:
#ifdef USE_SWITCH
        custom::CustomSwitch* min_heat_switch = new custom::CustomSwitch([this](bool state) { return this->controller_->controller->set_min_heat(this->indoor_unit_, state); });
        custom::CustomSwitch* rc_prohibit_switch = new custom::CustomSwitch([this](bool state) { return this->controller_->controller->set_rc_prohibit(this->indoor_unit_, state); });
#endif

        // Protocol uses zero based index for indoor units, but documentation does not...
        FujitsuGeneralAirStageHIndoorUnit(FujitsuGeneralAirStageHCentralController *controller, uint8_t indoor_unit) : controller_(controller), indoor_unit_(indoor_unit - 1) {}

        void setup() override;
        void dump_config() override;
        float get_setup_priority() const override { return esphome::setup_priority::DATA; }

        void control(const climate::ClimateCall& call) override;
        climate::ClimateTraits traits() override;

        void set_ignore_lock(bool ignore_lock) { this->ignore_lock_ = ignore_lock; }

    protected:
        FujitsuGeneralAirStageHCentralController* controller_{};
        uint8_t indoor_unit_{};
        bool ignore_lock_{};

    private:
        void update_from_device(const fujitsu_general::airstage::h::central_controller::Config& data);

        static constexpr climate::ClimateMode mode_to_climate_mode(fujitsu_general::airstage::h::central_controller::ModeEnum mode) noexcept;
        static constexpr climate::ClimateFanMode fan_speed_to_climate_fan_mode(fujitsu_general::airstage::h::central_controller::FanSpeedEnum fan_speed) noexcept;

        static constexpr fujitsu_general::airstage::h::central_controller::ModeEnum climate_mode_to_mode(climate::ClimateMode mode) noexcept;
        static constexpr fujitsu_general::airstage::h::central_controller::FanSpeedEnum climate_fan_mode_to_fan_speed(climate::ClimateFanMode fan_speed) noexcept;
};

}
