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
  OM100(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *white_color, FloatOutput *white_brightness)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    white_color_ = white_color;
    white_brightness_ = white_brightness;
  }

  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supports_brightness(true);
    traits.set_supports_rgb(true);
    traits.set_supports_rgb_white_value(false);
    traits.set_supports_color_temperature(true);
    traits.set_supports_color_interlock(true); 
    traits.set_min_mireds(154); // 6500 K
    traits.set_max_mireds(370); // 2700 K
    return traits;
  }

  void write_state(LightState *state) override {
    float brightness, red, green, blue, white;

    auto traits = state->get_traits();
    state->current_values_as_rgbw(&red, &green, &blue, &white, traits.get_supports_color_interlock());
    state->current_values_as_brightness(&brightness);

    auto current_values = state->current_values;
    float color_temp = current_values.get_color_temperature();
    float color_temperature_cw = traits.get_min_mireds();
    float color_temperature_ww = traits.get_max_mireds();
    float color_temp_clamped = clamp(color_temp, color_temperature_cw, color_temperature_ww);
    float white_color = 1.0f - (color_temp_clamped - color_temperature_cw) / (color_temperature_ww - color_temperature_cw);

    // ESP_LOGD("write_state", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // red, green, blue, brightness, white_color, white);

    // ESP_LOGD("write_state_corrected", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // corrected_level(red), corrected_level(green), corrected_level(blue), brightness, white_color, corrected_level(white));

    this->red_->set_level(corrected_level(red));
    this->green_->set_level(corrected_level(green));
    this->blue_->set_level(corrected_level(blue));
    this->white_color_->set_level(white_color);
    this->white_brightness_->set_level(corrected_level(white));
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *white_color_;
    FloatOutput *white_brightness_;
};

