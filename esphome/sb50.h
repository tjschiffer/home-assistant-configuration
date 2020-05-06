#include "esphome.h"

int COLOR_GROUP_WHITE = 0;
int COLOR_GROUP_RGB = 1;
uint32_t DEFAULT_TRANSITION_LENGTH = 200; // 0.2s
float MIN_MIREDS = 161; // 6200 K
float MAX_MIREDS = 357; // 2800 K

class SB50 : public Component, public LightOutput {
  public:
  SB50(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *cwhite, FloatOutput *wwhite)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    cwhite_ = cwhite;
    wwhite_ = wwhite;
  }

  LightTraits get_traits() override {
    auto traits = LightTraits();
    traits.set_supports_brightness(true);
    traits.set_supports_rgb(true);
    traits.set_supports_rgb_white_value(false);
    traits.set_supports_color_temperature(true);
    traits.set_min_mireds(MIN_MIREDS); 
    traits.set_max_mireds(MAX_MIREDS); 
    return traits;
  }

  float get_progress() { 
    if (!this->transition_start_time_)
      return 0.0f;
    return clamp((millis() - this->transition_start_time_) / float(DEFAULT_TRANSITION_LENGTH), 0.0f, 1.0f); 
  }

  void remote_state_change(LightState *state) {
    auto remote_values = state->remote_values;

    this->prev_corrected_output_= this->corrected_output_;

    float red = remote_values.get_red();
    float green = remote_values.get_green();
    float blue = remote_values.get_blue();
    float color_temp = remote_values.get_color_temperature();
    if (red != this->prev_red_ || green != this->prev_green_ || blue != this->prev_blue_) {
      this->current_color_group_ = COLOR_GROUP_RGB;
    } else if (color_temp && color_temp != this->color_temp_) {
      this->current_color_group_ = COLOR_GROUP_WHITE;
    }

    if (this->current_color_group_ == COLOR_GROUP_RGB) {
      this->prev_red_ = red;
      this->prev_green_ = green;
      this->prev_blue_ = blue;
      this->corrected_output_["red"] = red;
      this->corrected_output_["green"] = green;
      this->corrected_output_["blue"] = blue;
      this->corrected_output_["cold_white"] = 0.0f;
      this->corrected_output_["warm_white"] = 0.0f;
    }

    if (this->current_color_group_ == COLOR_GROUP_WHITE) {
      this->color_temp_ = color_temp;

      auto traits = state->get_traits();
      float color_temperature_cw = traits.get_min_mireds();
      float color_temperature_ww = traits.get_max_mireds();
      float color_temp_clamped = clamp(color_temp, color_temperature_cw, color_temperature_ww);
      float ww_fraction = (color_temp_clamped - color_temperature_cw) / (color_temperature_ww - color_temperature_cw);
      float cw_fraction = 1.0f - ww_fraction;
      float max_cw_ww = std::max(ww_fraction, cw_fraction);
      float cold_white = (cw_fraction / max_cw_ww);
      float warm_white = (ww_fraction / max_cw_ww);

      this->corrected_output_["red"] = 0.0f;
      this->corrected_output_["green"] = 0.0f;
      this->corrected_output_["blue"] = 0.0f;
      this->corrected_output_["cold_white"] = cold_white;
      this->corrected_output_["warm_white"] = warm_white;
    }

    // Adjust for off state, which will 0 all values. Multiply by brightness.
    float state_on_off = remote_values.get_state();
    float brightness = remote_values.get_brightness();
    for ( auto &s : this->corrected_output_ ) s.second *= (state_on_off * brightness);
  }

  void setup_state(LightState *state) override {
    state->add_new_remote_values_callback([this, state]() { this->remote_state_change(state); });
    state->set_default_transition_length(DEFAULT_TRANSITION_LENGTH);
  }

  void write_state(LightState *state) override {
    auto current_values = state->current_values;
    
    if (this->transition_start_time_ == 0) {
      this->transition_start_time_ = millis();
    };

    float progress = this->get_progress();
    // transition complete
    if (current_values == state->remote_values) {
      this->transition_start_time_ = 0;
      progress = 1.0f;
    }
  
    float lerp_red, lerp_green, lerp_blue, lerp_cold_white, lerp_warm_white;
    lerp_red = esphome::lerp(progress, this->prev_corrected_output_["red"], this->corrected_output_["red"]);
    lerp_green = esphome::lerp(progress, this->prev_corrected_output_["green"], this->corrected_output_["green"]);
    lerp_blue = esphome::lerp(progress, this->prev_corrected_output_["blue"], this->corrected_output_["blue"]);
    lerp_cold_white = esphome::lerp(progress, this->prev_corrected_output_["cold_white"], this->corrected_output_["cold_white"]);
    lerp_warm_white = esphome::lerp(progress, this->prev_corrected_output_["warm_white"], this->corrected_output_["warm_white"]);

    // ESP_LOGD("write_state",
    //     "red %f, green %f, blue %f, cwhite %f, wwhite %f", lerp_red, lerp_green, lerp_blue, lerp_cold_white, lerp_warm_white
    // );

    this->red_->set_level(lerp_red);
    this->green_->set_level(lerp_green);
    this->blue_->set_level(lerp_blue);
    this->cwhite_->set_level(lerp_cold_white);
    this->wwhite_->set_level(lerp_warm_white);
  }

  protected:
    FloatOutput *red_;
    FloatOutput *green_;
    FloatOutput *blue_;
    FloatOutput *cwhite_;
    FloatOutput *wwhite_;

    uint32_t transition_start_time_{0};

    int current_color_group_;

    float color_temp_;
    float prev_red_;
    float prev_green_;
    float prev_blue_;
    
    std::map<std::string, float> corrected_output_;
    std::map<std::string, float> prev_corrected_output_;
};