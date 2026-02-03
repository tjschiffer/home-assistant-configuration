#include "esphome.h"

// float MIN_OUTPUT = 0.0006;

// float corrected_level(float value) {
//   if (value <= 0.000001) {
//     return 0.0f;
//   }
//   return max(value, MIN_OUTPUT);
// }

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
    auto traits = light::LightTraits();
    traits.set_supported_color_modes({light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
    traits.set_min_mireds(161); // 6200 K
    traits.set_max_mireds(357); // 2800 K
    return traits;
  }

  void write_state(LightState *state) override {
    // float brightness, red, green, blue, white;

    // auto traits = state->get_traits();
    // state->current_values_as_rgbw(&red, &green, &blue, &white, traits.get_supports_color_interlock());
    // state->current_values_as_brightness(&brightness);

    // auto current_values = state->current_values;
    // float color_temp = current_values.get_color_temperature();
    // float color_temperature_cw = traits.get_min_mireds();
    // float color_temperature_ww = traits.get_max_mireds();
    // float color_temp_clamped = clamp(color_temp, color_temperature_cw, color_temperature_ww);
    // float white_color = 1.0f - (color_temp_clamped - color_temperature_cw) / (color_temperature_ww - color_temperature_cw);

    float red, green, blue, color_temperature, white_brightness;
    state->current_values_as_rgbct(&red, &green, &blue, &color_temperature, &white_brightness);

    float adjustedRed = 0.0f;
    float adjustedGreen = 0.0f;
    float adjustedBlue = 0.0f;
    float correctedWhite = white_brightness;
    if (white_brightness > 0.0f) {
      float diff = fabs(color_temperature - 0.5f);
      float completion = diff * 2.0f;
      correctedWhite = esphome::lerp(completion, 1.0f, 0.8f) * white_brightness;
      if (color_temperature < 0.5f) {
        adjustedGreen = esphome::lerp(completion, 0.0f, 0.2f) * white_brightness;
        adjustedBlue = esphome::lerp(completion, 0.0f, 0.2f) * white_brightness;
      } else {
        adjustedRed = esphome::lerp(completion, 0.0f, 0.5f) * white_brightness;
      }
    }

    float correctedRed = min(red + adjustedRed, 1.0f);
    float correctedGreen = min(green + adjustedGreen, 1.0f);
    float correctedBlue = min(blue + adjustedBlue, 1.0f);

    // ESP_LOGD("read_state", "R:%f G:%f B:%f Br:%f WC:%f WBr:%f", 
    // red, green, blue, brightness, white_color, white);

    // ESP_LOGD("write_state", "R:%f G:%f B:%f W:%f", 
    // correctedRed, correctedGreen, correctedBlue, correctedWhite);

    this->red_->set_level(correctedRed);
    this->green_->set_level(correctedGreen);
    this->blue_->set_level(correctedBlue);
    this->white_->set_level(correctedWhite);
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *white_;
};

