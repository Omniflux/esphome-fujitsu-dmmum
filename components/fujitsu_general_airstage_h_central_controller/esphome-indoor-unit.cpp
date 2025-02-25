#include <ranges>

#include "esphome-indoor-unit.h"

namespace esphome::fujitsu_general_airstage_h_central_controller {

static const auto TAG = "esphome::fujitsu_general_airstage_h_central_controller::indoor_unit";

void FujitsuGeneralAirStageHIndoorUnit::setup() {
    this->controller_->set_config_callback(this->indoor_unit_, [this](const fujitsu_general::airstage::h::central_controller::Config& data){ this->update_from_device(data); });

    if (this->temperature_sensor_)
        this->temperature_sensor_->add_on_raw_state_callback([this](float temperature) {
            this->current_temperature = temperature;
            this->publish_state();
        });
}

void FujitsuGeneralAirStageHIndoorUnit::dump_config() {
    LOG_CLIMATE("", "FujitsuGeneralAirStageHIndoorUnit", this);
    ESP_LOGCONFIG(TAG, "  Indoor Unit: %u", this->indoor_unit_ + 1);
    ESP_LOGCONFIG(TAG, "  Ignore Lock: %s", this->ignore_lock_ ? "YES" : "NO");
    LOG_SENSOR("  ", "Temperature Sensor", this->temperature_sensor_);
    this->dump_traits_(TAG);
}

climate::ClimateTraits FujitsuGeneralAirStageHIndoorUnit::traits() {
    using namespace climate;

    auto traits = ClimateTraits();

    // Target temperature / Setpoint
    traits.set_visual_temperature_step(1);
    traits.set_visual_min_temperature(fujitsu_general::airstage::h::central_controller::MinSetpoint);
    traits.set_visual_max_temperature(fujitsu_general::airstage::h::central_controller::MaxSetpoint);

    // Current temperature
    traits.set_supports_current_temperature(this->temperature_sensor_ != nullptr);

    // Mode
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_HEAT_COOL);
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_HEAT);
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_DRY);
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_COOL);

    // Fan mode / speed
    traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_QUIET);
    traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
    traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
    traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
    traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);

    // Economy mode
    traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);
    traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_ECO);

    return traits;
}

void FujitsuGeneralAirStageHIndoorUnit::control(const climate::ClimateCall& call) {
    using climate::ClimateMode;
    using climate::ClimatePreset;

    // Target temperature / Setpoint
    if (call.get_target_temperature().has_value())
        this->controller_->controller->set_setpoint(this->indoor_unit_, call.get_target_temperature().value(), this->ignore_lock_);

    // Economy mode
    if (call.get_preset().has_value())
        this->controller_->controller->set_economy(this->indoor_unit_, call.get_preset().value() == ClimatePreset::CLIMATE_PRESET_ECO, this->ignore_lock_);

    // Fan mode / speed
    if (call.get_fan_mode().has_value())
        this->controller_->controller->set_fan_speed(this->indoor_unit_, climate_fan_mode_to_fan_speed(call.get_fan_mode().value()), this->ignore_lock_);

    // Mode / enabled
    if (call.get_mode().has_value()) {
        if (call.get_mode().value() == ClimateMode::CLIMATE_MODE_OFF)
            this->controller_->controller->set_enabled(this->indoor_unit_, false, this->ignore_lock_);
        else {
            this->controller_->controller->set_enabled(this->indoor_unit_, true, this->ignore_lock_);
            this->controller_->controller->set_mode(this->indoor_unit_, climate_mode_to_mode(call.get_mode().value()), this->ignore_lock_);
        }
    }

    this->publish_state();
}

void FujitsuGeneralAirStageHIndoorUnit::update_from_device(const fujitsu_general::airstage::h::central_controller::Config& data) {
    using climate::ClimateFanMode;
    using climate::ClimateMode;
    using climate::ClimatePreset;

    auto need_to_publish = false;

    // Error sensor
    if (!this->error_sensor->has_state())
        this->error_sensor->publish_state(data.OutdoorUnit.Error);

    // Standby mode sensor
    // This can indicate defrosting, performing oil recovery, waiting for other units to complete....
    if (!this->standby_sensor->has_state() || data.OutdoorUnit.StandbyMode != this->standby_sensor->state)
        this->standby_sensor->publish_state(data.OutdoorUnit.StandbyMode);

    // Minimum Heat mode    // TODO May need to clear eco mode
    if (data.OutdoorUnit.MinHeat != this->min_heat_switch->state)
        this->min_heat_switch->publish_state(data.OutdoorUnit.MinHeat);

    // RC Prohibit switch
    if (data.OutdoorUnit.RCProhibit != this->rc_prohibit_switch->state)
        this->rc_prohibit_switch->publish_state(data.OutdoorUnit.RCProhibit);

    // Economy mode
    if (data.OutdoorUnit.Economy != (this->preset == ClimatePreset::CLIMATE_PRESET_ECO)) {
        this->preset = data.OutdoorUnit.Economy ? ClimatePreset::CLIMATE_PRESET_ECO : ClimatePreset::CLIMATE_PRESET_NONE;
        need_to_publish = true;
    }

    // Target temperature / Setpoint
    if (data.Setpoint != this->target_temperature) {
        this->target_temperature = data.Setpoint;
        need_to_publish = true;
    }

    // Fan mode / speed
    const auto fan_mode = fan_speed_to_climate_fan_mode(data.FanSpeed);
    if (fan_mode != this->fan_mode) {
        this->fan_mode = fan_mode;
        need_to_publish = true;
    }

    // Mode / enabled
    const auto mode = data.OutdoorUnit.Enabled ? mode_to_climate_mode(data.Mode) : ClimateMode::CLIMATE_MODE_OFF;
    if (mode != this->mode) {
        this->mode = mode;
        need_to_publish = true;
    }

    if (need_to_publish)
        this->publish_state();
}

constexpr climate::ClimateMode FujitsuGeneralAirStageHIndoorUnit::mode_to_climate_mode(const fujitsu_general::airstage::h::central_controller::ModeEnum mode) {
    using climate::ClimateMode;
    using FujitsuMode = fujitsu_general::airstage::h::central_controller::ModeEnum;

    switch (mode) {
        case FujitsuMode::Fan:  return ClimateMode::CLIMATE_MODE_FAN_ONLY;
        case FujitsuMode::Dry:  return ClimateMode::CLIMATE_MODE_DRY;
        case FujitsuMode::Cool: return ClimateMode::CLIMATE_MODE_COOL;
        case FujitsuMode::Heat: return ClimateMode::CLIMATE_MODE_HEAT;
        case FujitsuMode::Auto: return ClimateMode::CLIMATE_MODE_HEAT_COOL;

        // Should not get to this point
        default: return ClimateMode::CLIMATE_MODE_FAN_ONLY;
    }
}

constexpr climate::ClimateFanMode FujitsuGeneralAirStageHIndoorUnit::fan_speed_to_climate_fan_mode(const fujitsu_general::airstage::h::central_controller::FanSpeedEnum fan_speed) {
    using climate::ClimateFanMode;
    using FujitsuFanMode = fujitsu_general::airstage::h::central_controller::FanSpeedEnum;

    switch (fan_speed) {
        case FujitsuFanMode::Auto:   return ClimateFanMode::CLIMATE_FAN_AUTO;
        case FujitsuFanMode::Quiet:  return ClimateFanMode::CLIMATE_FAN_QUIET;
        case FujitsuFanMode::Low:    return ClimateFanMode::CLIMATE_FAN_LOW;
        case FujitsuFanMode::Medium: return ClimateFanMode::CLIMATE_FAN_MEDIUM;
        case FujitsuFanMode::High:   return ClimateFanMode::CLIMATE_FAN_HIGH;

        // Should not get to this point
        default: return ClimateFanMode::CLIMATE_FAN_AUTO;
    }
}

constexpr fujitsu_general::airstage::h::central_controller::ModeEnum FujitsuGeneralAirStageHIndoorUnit::climate_mode_to_mode(climate::ClimateMode mode) {
    using climate::ClimateMode;
    using FujitsuMode = fujitsu_general::airstage::h::central_controller::ModeEnum;

    switch (mode) {
        case ClimateMode::CLIMATE_MODE_HEAT_COOL: return FujitsuMode::Auto;
        case ClimateMode::CLIMATE_MODE_COOL:      return FujitsuMode::Cool;
        case ClimateMode::CLIMATE_MODE_HEAT:      return FujitsuMode::Heat;
        case ClimateMode::CLIMATE_MODE_FAN_ONLY:  return FujitsuMode::Fan;
        case ClimateMode::CLIMATE_MODE_DRY:       return FujitsuMode::Dry;

        // Should not get to this point if traits is respected
        default: return FujitsuMode::Fan;
    }
} 

constexpr fujitsu_general::airstage::h::central_controller::FanSpeedEnum FujitsuGeneralAirStageHIndoorUnit::climate_fan_mode_to_fan_speed(climate::ClimateFanMode fan_speed) {
    using climate::ClimateFanMode;
    using FujitsuFanMode = fujitsu_general::airstage::h::central_controller::FanSpeedEnum;

    switch (fan_speed) {
        case ClimateFanMode::CLIMATE_FAN_AUTO:   return FujitsuFanMode::Auto;
        case ClimateFanMode::CLIMATE_FAN_LOW:    return FujitsuFanMode::Low;
        case ClimateFanMode::CLIMATE_FAN_MEDIUM: return FujitsuFanMode::Medium;
        case ClimateFanMode::CLIMATE_FAN_HIGH:   return FujitsuFanMode::High;
        case ClimateFanMode::CLIMATE_FAN_QUIET:  return FujitsuFanMode::Quiet;

        // Should not get to this point if traits is respected
        default: return FujitsuFanMode::Auto;
    }
}

}