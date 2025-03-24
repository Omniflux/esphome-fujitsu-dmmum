#pragma once

#include <functional>

#include <esphome/core/component.h>
#include <esphome/components/button/button.h>

namespace esphome::fujitsu_general_airstage_h_central_controller {

class CustomButton : public Component, public button::Button {
    public:
        CustomButton(std::function<void()> func) : func(func) {};
        void press_action() override { this->func(); };

    private:
        CustomButton() {};
        std::function<void()> func;
};
    
}