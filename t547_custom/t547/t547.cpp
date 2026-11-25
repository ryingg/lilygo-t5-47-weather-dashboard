#include "t547.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "logo.h"

#ifdef USE_ESP32_FRAMEWORK_ARDUINO

#include <esp32-hal-gpio.h>

namespace esphome {
namespace t547 {

static const char *const TAG = "t574";

void T547::setup() {
  this->initialize_();
  // Intentionally not drawing here: at this point in boot, WiFi/API hasn't
  // connected yet and Home Assistant sensor values are still unknown/nan.
  // The YAML's own component.update call (fired once real data arrives via
  // the all_data_received script) is the single draw per wake/refresh cycle.
}

void T547::initialize_() {
  ESP_LOGV(TAG, "Initialize called");
  uint32_t buffer_size = this->get_buffer_length_();

  if (this->buffer_ != nullptr) {
    free(this->buffer_);  // NOLINT

  epd_init();
  }

  this->buffer_ = (uint8_t *) ps_malloc(buffer_size);

  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate buffer for display!");
    this->mark_failed();
    return;
  }
  uint8_t background = invert_ ? 255 : 0;
  memset(this->buffer_, background, buffer_size);
  ESP_LOGV(TAG, "Initialize complete");
}

float T547::get_setup_priority() const { return setup_priority::PROCESSOR; }
size_t T547::get_buffer_length_() {
    return this->get_width_internal() * this->get_height_internal() / 2;
}

void T547::update() {
  this->do_update_();
  this->display();
}

void HOT T547::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || y >= this->get_height_internal() || x < 0 || y < 0)
    return;
  uint8_t gs = ((color.red * 2126 / 10000) + (color.green * 7152 / 10000) + (color.blue * 722 / 10000));
  if (invert_) {
    gs = 255 - gs;
  }
  epd_draw_pixel(x, y, gs, this->buffer_);
}

void T547::dump_config() {
  LOG_DISPLAY("", "T547", this);
  LOG_UPDATE_INTERVAL(this);
}

void T547::eink_off_() {
  ESP_LOGV(TAG, "Eink off called");
  if (panel_on_ == 0)
    return;
  epd_poweroff();
  panel_on_ = 0;
}

void T547::eink_on_() {
  ESP_LOGV(TAG, "Eink on called");
  if (panel_on_ == 1)
    return;
  epd_poweron();
  panel_on_ = 1;
}

void T547::display() {
  ESP_LOGV(TAG, "Display called");
  uint32_t start_time = millis();

  epd_poweron();
  if (this->greyscale_) {
    // Full 16-level grayscale: 4 clear cycles + 15 contrast frames.
    epd_clear();
    epd_draw_grayscale_image(epd_full_screen(), this->buffer_);
  } else {
    // Monochrome mode: moderate clear + contrast frame count, tuned between
    // the stock 15-frame (too dark/bloomy, ~12 visible flashes) and an
    // earlier too-light 2-frame attempt. Adjust frame count up/down here
    // to taste.
    epd_clear_area_cycles(epd_full_screen(), 2, 50);
    epd_draw_image_frames(epd_full_screen(), this->buffer_, BLACK_ON_WHITE, 6);
  }
  epd_poweroff();

  ESP_LOGV(TAG, "Display finished (full) (%ums)", millis() - start_time);
}

void T547::clean() {
  ESP_LOGV(TAG, "Clean called");
  uint32_t start_time = millis();
  if (this->greyscale_) {
    epd_clear();
  } else {
    epd_clear_area_cycles(epd_full_screen(), 2, 50);
  }

  ESP_LOGV(TAG, "Clean finished (%ums)", millis() - start_time);
}
}  // namespace T547
}  // namespace esphome

#endif  // USE_ESP32_FRAMEWORK_ARDUINO
