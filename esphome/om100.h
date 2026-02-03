#include "esphome.h"

float MIN_OUTPUT = 0.0006;

float corrected_level(float value) {
  if (value <= 0.000001) {
    return 0.0f;
  }
  return max(value, MIN_OUTPUT);
}

class OM100 : public Component, public LightOutput {
  public:
  OM100(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *white_temperature, FloatOutput *white_brightness)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    white_temperature_ = white_temperature;
    white_brightness_ = white_brightness;
  }

  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supported_color_modes({light::ColorMode::COLOR_TEMPERATURE, light::ColorMode::RGB });
    traits.set_min_mireds(154); // 6500 K
    traits.set_max_mireds(370); // 2700 K
    return traits;
  }

  void write_state(LightState *state) override {
    float red, green, blue, cwhite, wwhite;

    state->current_values_as_rgbww(&red, &green, &blue, &cwhite, &wwhite, true);

    auto traits = state->get_traits();
    auto current_values = state->current_values;
    float color_temp = current_values.get_color_temperature();
    float color_temperature_cw = traits.get_min_mireds();
    float color_temperature_ww = traits.get_max_mireds();
    float color_temp_clamped = clamp(color_temp, color_temperature_cw, color_temperature_ww);
    float white_temperature = 1.0f - (color_temp_clamped - color_temperature_cw) / (color_temperature_ww - color_temperature_cw);

    float brightness = current_values.get_brightness();

    // ESP_LOGD("write_state", "R:%f G:%f B:%f Br:%f WC:%f", 
    // red, green, blue, brightness, white_temperature);

    // ESP_LOGD("write_state_corrected", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // corrected_level(red), corrected_level(green), corrected_level(blue), brightness, white_temperature, corrected_level(white));

    this->red_->set_level(corrected_level(red));
    this->green_->set_level(corrected_level(green));
    this->blue_->set_level(corrected_level(blue));
    this->white_temperature_->set_level(white_temperature);
    this->white_brightness_->set_level(corrected_level(brightness));
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *white_temperature_;
    FloatOutput *white_brightness_;
};

