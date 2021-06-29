#include "esphome.h"

float MIN_OUTPUT = 0.0006;

float corrected_level(float value) {
  if (value <= 0.000001) {
    return 0.0f;
  }
  return max(value, MIN_OUTPUT);
}

class SB50_r : public Component, public LightOutput {
  public:
  SB50_r(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *white)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    white_ = white;
  }

  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supports_brightness(true);
    traits.set_supports_rgb(true);
    traits.set_supports_rgb_white_value(false);
    traits.set_supports_color_temperature(true);
    traits.set_supports_color_interlock(true); 
    traits.set_min_mireds(161); // 6200 K
    traits.set_max_mireds(357); // 2800 K
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

    float adjustedRed = 0.0f;
    float adjustedGreen = 0.0f;
    float adjustedBlue = 0.0f;
    float correctedWhite = white;
    if (white > 0.0f) {
      float diff = fabs(white_color - 0.5f);
      float completion = diff * 2.0f;
      correctedWhite = esphome::lerp(completion, 1.0f, 0.8f) * white;
      if (white_color > 0.5f) {
        adjustedGreen = esphome::lerp(completion, 0.0f, 0.2f) * white;
        adjustedBlue = esphome::lerp(completion, 0.0f, 0.2f) * white;
      } else {
        adjustedRed = esphome::lerp(completion, 0.0f, 0.5f) * white;
      }
    }

    float correctedRed = min(red + adjustedRed, 1.0f);
    float correctedGreen = min(green + adjustedGreen, 1.0f);
    float correctedBlue = min(blue + adjustedBlue, 1.0f);

    // ESP_LOGD("read_state", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // red, green, blue, brightness, white_color, white);

    // ESP_LOGD("write_state", "R:%f G:%f B:%f W:%f", 
    // correctedRed, correctedGreen, correctedBlue, correctedWhite);

    this->red_->set_level(corrected_level(correctedRed));
    this->green_->set_level(corrected_level(correctedGreen));
    this->blue_->set_level(corrected_level(correctedBlue));
    this->white_->set_level(corrected_level(correctedWhite));
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *white_;
};

