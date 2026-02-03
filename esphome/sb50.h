#include "esphome.h"

float MIN_OUTPUT = 0.0006;

float corrected_level(float value) {
  if (value <= 0.000001) {
    return 0.0f;
  }
  return max(value, MIN_OUTPUT);
}

class SB50 : public Component, public LightOutput {
  public:
  SB50(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *warm_white, FloatOutput *cold_white)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    warm_white_ = warm_white;
    cold_white_ = cold_white;
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
    float brightness, red, green, blue, cold_white, warm_white;

    auto traits = state->get_traits();
    state->current_values_as_rgbww(&red, &green, &blue, &cold_white, &warm_white, false, traits.get_supports_color_interlock());
    state->current_values_as_brightness(&brightness);

    // ESP_LOGD("write_state", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // red, green, blue, brightness, white_color, white);

    // ESP_LOGD("write_state_corrected", "R:%f G:%f B:%f WW:%f CW:%f Br:%f", 
    // corrected_level(red), corrected_level(green), corrected_level(blue),  corrected_level(warm_white),  corrected_level(cold_white), brightness);

    this->red_->set_level(corrected_level(red));
    this->green_->set_level(corrected_level(green));
    this->blue_->set_level(corrected_level(blue));
    this->warm_white_->set_level(corrected_level(warm_white));
    this->cold_white_->set_level(corrected_level(cold_white));
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *warm_white_;
    FloatOutput *cold_white_;
};

