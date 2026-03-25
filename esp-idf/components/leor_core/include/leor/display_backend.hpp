#pragma once

#include "leor/config.hpp"
#include "esp32_hw_i2c.h"

#include <cstdint>
#include <memory>
#include <string>

struct u8g2_struct;
typedef struct u8g2_struct u8g2_t;

namespace leor {

class DisplayBackend {
  public:
    virtual ~DisplayBackend() = default;

    virtual bool init(const DisplayConfig& config) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void clear() = 0;
    virtual void send_buffer() = 0;
    virtual void prepare_sleep() {}
    virtual void set_contrast(uint8_t value) = 0;
    virtual void set_color(uint8_t color) = 0;

    virtual void draw_pixel(int x, int y) = 0;
    virtual void draw_line(int x0, int y0, int x1, int y1) = 0;
    virtual void draw_hline(int x, int y, int w) = 0;
    virtual void draw_vline(int x, int y, int h) = 0;
    virtual void draw_box(int x, int y, int w, int h) = 0;
    virtual void draw_frame(int x, int y, int w, int h) = 0;
    virtual void draw_rbox(int x, int y, int w, int h, int r) = 0;
    virtual void draw_rframe(int x, int y, int w, int h, int r) = 0;
    virtual void draw_disc(int x, int y, int r) = 0;
    virtual void draw_circle(int x, int y, int r) = 0;

    virtual void fill_box(int x, int y, int w, int h) = 0;
    virtual void fill_rbox(int x, int y, int w, int h, int r) = 0;
    virtual void fill_circle(int x, int y, int r) = 0;
    virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2) = 0;
    virtual void fill_round_rect(int x, int y, int w, int h, int r) = 0;

    virtual void set_font_small() = 0;
    virtual void set_font_medium() = 0;
    virtual void set_font_large() = 0;
    virtual void draw_text(int x, int y, const char* text) = 0;
    virtual int text_width(const char* text) = 0;
};

class NullDisplayBackend final : public DisplayBackend {
  public:
    bool init(const DisplayConfig& config) override;
    int width() const override { return width_; }
    int height() const override { return height_; }
    void clear() override {}
    void send_buffer() override {}
    void set_contrast(uint8_t) override {}
    void set_color(uint8_t) override {}
    void draw_pixel(int, int) override {}
    void draw_line(int, int, int, int) override {}
    void draw_hline(int, int, int) override {}
    void draw_vline(int, int, int) override {}
    void draw_box(int, int, int, int) override {}
    void draw_frame(int, int, int, int) override {}
    void draw_rbox(int, int, int, int, int) override {}
    void draw_rframe(int, int, int, int, int) override {}
    void draw_disc(int, int, int) override {}
    void draw_circle(int, int, int) override {}
    void fill_box(int, int, int, int) override {}
    void fill_rbox(int, int, int, int, int) override {}
    void fill_circle(int, int, int) override {}
    void fill_triangle(int, int, int, int, int, int) override {}
    void fill_round_rect(int, int, int, int, int) override {}
    void set_font_small() override {}
    void set_font_medium() override {}
    void set_font_large() override {}
    void draw_text(int, int, const char*) override {}
    int text_width(const char*) override { return 0; }

  private:
    int width_ = 128;
    int height_ = 64;
};

class U8g2DisplayBackend final : public DisplayBackend {
  public:
    U8g2DisplayBackend();
    ~U8g2DisplayBackend() override;

    bool init(const DisplayConfig& config) override;
    int width() const override;
    int height() const override;
    void clear() override;
    void send_buffer() override;
    void prepare_sleep() override;
    void set_contrast(uint8_t value) override;
    void set_color(uint8_t color) override;
    void draw_pixel(int x, int y) override;
    void draw_line(int x0, int y0, int x1, int y1) override;
    void draw_hline(int x, int y, int w) override;
    void draw_vline(int x, int y, int h) override;
    void draw_box(int x, int y, int w, int h) override;
    void draw_frame(int x, int y, int w, int h) override;
    void draw_rbox(int x, int y, int w, int h, int r) override;
    void draw_rframe(int x, int y, int w, int h, int r) override;
    void draw_disc(int x, int y, int r) override;
    void draw_circle(int x, int y, int r) override;

    void fill_box(int x, int y, int w, int h) override;
    void fill_rbox(int x, int y, int w, int h, int r) override;
    void fill_circle(int x, int y, int r) override;
    void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2) override;
    void fill_round_rect(int x, int y, int w, int h, int r) override;

    void set_font_small() override;
    void set_font_medium() override;
    void set_font_large() override;
    void draw_text(int x, int y, const char* text) override;
    int text_width(const char* text) override;

  private:
    void select_font(const uint8_t* font);
    u8g2_t* handle_ = nullptr;
    u8g2_esp32_i2c_ctx_t* i2c_ctx_ = nullptr;
    int width_ = 128;
    int height_ = 64;
    std::unique_ptr<uint8_t[]> storage_;
};

}  // namespace leor
