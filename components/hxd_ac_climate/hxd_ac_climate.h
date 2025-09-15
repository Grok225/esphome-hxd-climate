#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/globals/globals_component.h"

namespace esphome {
namespace hxd_ac {

class HXD_AC_Climate : public climate::Climate, public Component {
 public:
  void set_uart_bus(uart::UARTComponent *uart_bus) { this->uart_bus_ = uart_bus; }
  void set_high_byte_global(globals::GlobalsComponent<uint8_t> *global) { this->high_byte_global_ = global; }
  void set_low_byte_global(globals::GlobalsComponent<uint8_t> *global) { this->low_byte_global_ = global; }

  void setup() override {
    this->traits_.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_AUTO,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_DRY,
        climate::CLIMATE_MODE_FAN_ONLY
    });

    this->traits_.set_supported_fan_modes({
        climate::CLIMATE_FAN_AUTO,
        climate::CLIMATE_FAN_LOW,
        climate::CLIMATE_FAN_MEDIUM,
        climate::CLIMATE_FAN_HIGH
    });

    this->traits_.set_visual_min_temperature(16);
    this->traits_.set_visual_max_temperature(30);
    this->traits_.set_visual_temperature_step(1.0f);
  }

  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value()) {
      this->mode = call.get_mode().value();
    }
    if (call.get_target_temperature().has_value()) {
      this->target_temperature = call.get_target_temperature().value();
    }
    if (call.get_fan_mode().has_value()) {
      this->fan_mode = call.get_fan_mode().value();
    }

    send_command();
    this->publish_state();
  }

  climate::ClimateTraits traits() override { return this->traits_; }

 protected:
  void send_command() {
    uint8_t hb = this->high_byte_global_->value();
    uint8_t lb = this->low_byte_global_->value();
    uint8_t command[5] = {0x30, 0x06, hb, lb, 0x00};
    bool command_set = false;

    if (this->mode == climate::CLIMATE_MODE_OFF) {
      command[4] = 0x80;
      command_set = true;
    } else if (this->target_temperature.has_value()) {
      int temp = (int)this->target_temperature.value();
      if (temp >= 16 && temp <= 30) {
        command[4] = (temp - 16) + 0x40;
        command_set = true;
      }
    } else if (this->fan_mode.has_value()) {
      switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_AUTO:   command[4] = 0x51; break;
        case climate::CLIMATE_FAN_LOW:    command[4] = 0x52; break;
        case climate::CLIMATE_FAN_MEDIUM: command[4] = 0x53; break;
        case climate::CLIMATE_FAN_HIGH:   command[4] = 0x54; break;
        default: break;
      }
      command_set = true;
    } else {
      switch (this->mode) {
        case climate::CLIMATE_MODE_AUTO:    command[4] = 0xA1; break;
        case climate::CLIMATE_MODE_COOL:    command[4] = 0xA2; break;
        case climate::CLIMATE_MODE_DRY:     command[4] = 0xA3; break;
        case climate::CLIMATE_MODE_FAN_ONLY: command[4] = 0xA4; break;
        case climate::CLIMATE_MODE_HEAT:    command[4] = 0xA5; break;
        default: break;
      }
      command_set = true;
    }

    if (!command_set) {
      ESP_LOGW("hxd_ac", "No valid command to send.");
      return;
    }

    this->uart_bus_->write_array(command, sizeof(command));
    ESP_LOGD("hxd_ac", "Sent IR command: %s", format_hex_pretty(command, sizeof(command)).c_str());
  }

  uart::UARTComponent *uart_bus_ {nullptr};
  globals::GlobalsComponent<uint8_t> *high_byte_global_ {nullptr};
  globals::GlobalsComponent<uint8_t> *low_byte_global_ {nullptr};
  climate::ClimateTraits traits_;
};

}  // namespace hxd_ac
}  // namespace esphome