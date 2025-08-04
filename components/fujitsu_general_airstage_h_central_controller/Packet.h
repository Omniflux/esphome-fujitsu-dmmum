#pragma once

#include <array>
#include <bit>
#include <cstdint>

namespace fujitsu_general::airstage::h::central_controller {

// Temperature is stored in packet as 4 degrees C less than actual
constexpr uint8_t TemperatureOffset = 4;

enum class AddressTypeEnum : uint8_t {
    OutdoorUnit = 1,
    Controller,
    BranchBox
};

enum class FanSpeedEnum : uint8_t {
    NoChange,
    Auto,
    High,
    Medium,
    Low,
    Quiet
};

enum class ModeEnum : uint8_t {
    NoChange,
    Cool,
    Dry,
    Heat,
    Fan,
    Auto
};

enum class TriStateEnum : uint8_t {
    NoChange,
    False,
    True
};
constexpr TriStateEnum boolToTriState(bool b) {
    return b ? TriStateEnum::True : TriStateEnum::False;
}

struct Config {
    struct OutdoorUnit {
        uint8_t IndoorUnit;
        bool Enabled;
        bool MinHeat;
        bool LowNoise;
        bool Economy;
        bool OpRestricted;
        bool Error;
        bool IncompatibleMode;
        bool TestRun;
        bool RCProhibit;

        constexpr bool operator==(const OutdoorUnit&) const = default;
    } OutdoorUnit;

    struct Controller {
        TriStateEnum Enabled;
        TriStateEnum Economy;
        TriStateEnum MinHeat;
        TriStateEnum LowNoise;
        TriStateEnum RCProhibit;

        constexpr bool operator==(const Controller&) const = default;
    } Controller;

    FanSpeedEnum FanSpeed;
    ModeEnum Mode;
    uint8_t Setpoint;
    uint8_t IndoorUnitBitfield;

    constexpr bool operator==(const Config&) const = default;
};

struct ByteMaskShiftData {
    constexpr ByteMaskShiftData(uint8_t byte, uint8_t mask) : byte(byte), mask(mask), shift(std::countr_zero(mask)) {};

    const uint8_t byte;
    const uint8_t mask;
    const uint8_t shift;
};

constexpr struct BMS {
    // These almost certainly are actually more than one field...
    // Type Controller, Outdoor Unit, Branch Box? + Branch Box Address?
    // I do not have Branch Boxes to test with...
    constexpr static auto SourceType                 = ByteMaskShiftData(0, 0b00110000);
    constexpr static auto SourceAddress              = ByteMaskShiftData(0, 0b00001100);
    constexpr static auto TokenDestinationType       = ByteMaskShiftData(1, 0b00110000);
    constexpr static auto TokenDestinationAddress    = ByteMaskShiftData(1, 0b00001100);

    constexpr static struct Config {
        constexpr static struct OutdoorUnit {
            constexpr static auto IndoorUnitBitfield = ByteMaskShiftData(3, 0b11111100);
            constexpr static auto IndoorUnit         = ByteMaskShiftData(4, 0b01110000);
            constexpr static auto FanSpeed           = ByteMaskShiftData(5, 0b11100000);
            constexpr static auto Mode               = ByteMaskShiftData(5, 0b00011100);
            constexpr static auto Enabled            = ByteMaskShiftData(5, 0b00000010);
            constexpr static auto MinHeat            = ByteMaskShiftData(6, 0b10000000);
            constexpr static auto LowNoise           = ByteMaskShiftData(6, 0b01000000);
            constexpr static auto Economy            = ByteMaskShiftData(6, 0b00100000);
            constexpr static auto Setpoint           = ByteMaskShiftData(6, 0b00011111);
            constexpr static auto OpRestricted       = ByteMaskShiftData(7, 0b00010000);
            constexpr static auto Error              = ByteMaskShiftData(7, 0b00001000);
            constexpr static auto IncompatibleMode   = ByteMaskShiftData(7, 0b00000100);
            constexpr static auto TestRun            = ByteMaskShiftData(7, 0b00000010);
            constexpr static auto RCProhibit         = ByteMaskShiftData(7, 0b00000001);
        } OutdoorUnit = {};

        constexpr static struct Controller {
            constexpr static auto IndoorUnitBitfield = ByteMaskShiftData(3, 0b11111111);
            constexpr static auto Mode               = ByteMaskShiftData(4, 0b11100000);
            constexpr static auto Enabled            = ByteMaskShiftData(4, 0b00011000);
            constexpr static auto Setpoint           = ByteMaskShiftData(5, 0b11111000);
            constexpr static auto FanSpeed           = ByteMaskShiftData(5, 0b00000111);
            constexpr static auto RCProhibit         = ByteMaskShiftData(6, 0b11000000);
            constexpr static auto MinHeat            = ByteMaskShiftData(6, 0b00110000);
            constexpr static auto LowNoise           = ByteMaskShiftData(6, 0b00001100);
            constexpr static auto Economy            = ByteMaskShiftData(6, 0b00000011);
        } Controller = {};
    } Config = {};
} BMS;
static_assert(BMS.Config.OutdoorUnit.LowNoise.shift == 6 && BMS.Config.Controller.LowNoise.shift == 2, "Shift values calculated incorrectly");

class Packet {
    public:
        static constexpr uint8_t FrameSize = 8;
        using Buffer = std::array<uint8_t, FrameSize>;

        Packet() {};
        Packet(Buffer buffer);
        Buffer to_buffer() const;

        AddressTypeEnum SourceType {};
        uint8_t         SourceAddress {};
        AddressTypeEnum TokenDestinationType {};
        uint8_t         TokenDestinationAddress {};

        struct Config Config {};

        static void invert_buffer(Buffer& buffer) { *reinterpret_cast<uint64_t*>(buffer.data()) = ~*reinterpret_cast<uint64_t*>(buffer.data()); };
};

}
