#include "Packet.h"

namespace fujitsu_general::airstage::h::central_controller {

Packet::Packet(Buffer buffer) {
    auto getField = [&buffer](const ByteMaskShiftData& bms) -> uint8_t {
        return (buffer[bms.byte] & bms.mask) >> bms.shift;
    };

    this->invert_buffer(buffer);

    this->SourceType = static_cast<AddressTypeEnum>(getField(BMS.SourceType));
    this->SourceAddress = getField(BMS.SourceAddress);
    this->TokenDestinationType = static_cast<AddressTypeEnum>(getField(BMS.TokenDestinationType));
    this->TokenDestinationAddress = getField(BMS.TokenDestinationAddress);

    switch (this->SourceType) {
        case AddressTypeEnum::BranchBox:
        case AddressTypeEnum::OutdoorUnit:
            this->Config.IndoorUnitBitfield = getField(BMS.Config.OutdoorUnit.IndoorUnitBitfield);
            this->Config.OutdoorUnit.IndoorUnit = getField(BMS.Config.OutdoorUnit.IndoorUnit);

            this->Config.Mode = static_cast<ModeEnum>(getField(BMS.Config.OutdoorUnit.Mode));
            this->Config.FanSpeed = static_cast<FanSpeedEnum>(getField(BMS.Config.OutdoorUnit.FanSpeed));
            this->Config.Setpoint = getField(BMS.Config.OutdoorUnit.Setpoint);
            if (this->Config.Setpoint)
                this->Config.Setpoint += TemperatureOffset;

            this->Config.OutdoorUnit.Enabled = getField(BMS.Config.OutdoorUnit.Enabled);

            this->Config.OutdoorUnit.MinHeat = getField(BMS.Config.OutdoorUnit.MinHeat);
            this->Config.OutdoorUnit.LowNoise = getField(BMS.Config.OutdoorUnit.LowNoise);
            this->Config.OutdoorUnit.Economy = getField(BMS.Config.OutdoorUnit.Economy);

            this->Config.OutdoorUnit.OpRestricted = getField(BMS.Config.OutdoorUnit.OpRestricted);
            this->Config.OutdoorUnit.Error = getField(BMS.Config.OutdoorUnit.Error);
            this->Config.OutdoorUnit.IncompatibleMode = getField(BMS.Config.OutdoorUnit.IncompatibleMode);
            this->Config.OutdoorUnit.TestRun = getField(BMS.Config.OutdoorUnit.TestRun);
            this->Config.OutdoorUnit.RCProhibit = getField(BMS.Config.OutdoorUnit.RCProhibit);

            break;

        case AddressTypeEnum::Controller:
            this->Config.IndoorUnitBitfield = getField(BMS.Config.Controller.IndoorUnitBitfield);

            this->Config.Mode = static_cast<ModeEnum>(getField(BMS.Config.Controller.Mode));
            this->Config.FanSpeed = static_cast<FanSpeedEnum>(getField(BMS.Config.Controller.FanSpeed));
            this->Config.Setpoint = getField(BMS.Config.Controller.Setpoint) + TemperatureOffset;
            if (this->Config.Setpoint)
                this->Config.Setpoint += TemperatureOffset;

            this->Config.Controller.Enabled = static_cast<TriStateEnum>(getField(BMS.Config.Controller.Enabled));

            this->Config.Controller.Economy = static_cast<TriStateEnum>(getField(BMS.Config.Controller.Economy));
            this->Config.Controller.MinHeat = static_cast<TriStateEnum>(getField(BMS.Config.Controller.MinHeat));
            this->Config.Controller.LowNoise = static_cast<TriStateEnum>(getField(BMS.Config.Controller.LowNoise));
            this->Config.Controller.RCProhibit = static_cast<TriStateEnum>(getField(BMS.Config.Controller.RCProhibit));

            break;
    }
};

Packet::Buffer Packet::to_buffer() const {
    Buffer buffer {};

    auto setField = [&buffer](const ByteMaskShiftData& bms, const auto value) {
        buffer[bms.byte] |= (value << bms.shift) & bms.mask;
    };

    setField(BMS.SourceType, static_cast<uint8_t>(this->SourceType));
    setField(BMS.SourceAddress, this->SourceAddress);

    setField(BMS.TokenDestinationType, static_cast<uint8_t>(this->TokenDestinationType));
    setField(BMS.TokenDestinationAddress, this->TokenDestinationAddress);

    buffer[1] |= 0b10000000; // Unknown bit set in all captured packets

    if (this->TokenDestinationType == AddressTypeEnum::Controller)
        buffer[1] |= 0b00000100; // Unknown bit set in all captured packets with controller dest token

    switch (this->SourceType) {
        case AddressTypeEnum::Controller:
            buffer[0] |= 0b00000100; // Unknown bit set in all captured packets from controller

            setField(BMS.Config.Controller.IndoorUnitBitfield, this->Config.IndoorUnitBitfield);
            setField(BMS.Config.Controller.Setpoint, this->Config.Setpoint ? this->Config.Setpoint - TemperatureOffset : 0);

            setField(BMS.Config.Controller.Mode, static_cast<uint8_t>(this->Config.Mode));
            setField(BMS.Config.Controller.FanSpeed, static_cast<uint8_t>(this->Config.FanSpeed));

            setField(BMS.Config.Controller.Enabled, static_cast<uint8_t>(this->Config.Controller.Enabled));
            setField(BMS.Config.Controller.RCProhibit, static_cast<uint8_t>(this->Config.Controller.RCProhibit));
            setField(BMS.Config.Controller.MinHeat, static_cast<uint8_t>(this->Config.Controller.MinHeat));
            setField(BMS.Config.Controller.LowNoise, static_cast<uint8_t>(this->Config.Controller.LowNoise));
            setField(BMS.Config.Controller.Economy, static_cast<uint8_t>(this->Config.Controller.Economy));

            break;

        case AddressTypeEnum::BranchBox:
            if (this->SourceAddress == 0)
                buffer[3] |= 0b00000001; // Unknown bit set in all captured packets from branch box 0
        case AddressTypeEnum::OutdoorUnit:
            setField(BMS.Config.OutdoorUnit.IndoorUnitBitfield, this->Config.IndoorUnitBitfield);
            setField(BMS.Config.OutdoorUnit.IndoorUnit, this->Config.OutdoorUnit.IndoorUnit);
            setField(BMS.Config.OutdoorUnit.Setpoint, this->Config.Setpoint ? this->Config.Setpoint - TemperatureOffset : 0);

            setField(BMS.Config.OutdoorUnit.Mode, static_cast<uint8_t>(this->Config.Mode));
            setField(BMS.Config.OutdoorUnit.FanSpeed, static_cast<uint8_t>(this->Config.FanSpeed));

            setField(BMS.Config.OutdoorUnit.Enabled, this->Config.OutdoorUnit.Enabled);
            setField(BMS.Config.OutdoorUnit.MinHeat, this->Config.OutdoorUnit.MinHeat);
            setField(BMS.Config.OutdoorUnit.LowNoise, this->Config.OutdoorUnit.LowNoise);
            setField(BMS.Config.OutdoorUnit.Economy, this->Config.OutdoorUnit.Economy);

            setField(BMS.Config.OutdoorUnit.OpRestricted, this->Config.OutdoorUnit.OpRestricted);
            setField(BMS.Config.OutdoorUnit.Error, this->Config.OutdoorUnit.Error);
            setField(BMS.Config.OutdoorUnit.IncompatibleMode, this->Config.OutdoorUnit.IncompatibleMode);
            setField(BMS.Config.OutdoorUnit.TestRun, this->Config.OutdoorUnit.TestRun);
            setField(BMS.Config.OutdoorUnit.RCProhibit, this->Config.OutdoorUnit.RCProhibit);

            break;
    }

    this->invert_buffer(buffer);

    return buffer;
};

}
