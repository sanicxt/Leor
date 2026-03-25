#include "leor/display_backend.hpp"

#include <algorithm>
#include <cmath>
#include "esp_err.h"
#include "esp_log.h"
#include "esp32_hw_i2c.h"
#include "u8g2.h"

namespace leor {

namespace {

static const char* kTag = "leor_display";

}  // namespace

bool NullDisplayBackend::init(const DisplayConfig& config) {
    width_ = config.width;
    height_ = config.height;
    return true;
}

U8g2DisplayBackend::U8g2DisplayBackend() = default;
U8g2DisplayBackend::~U8g2DisplayBackend() = default;

bool U8g2DisplayBackend::init(const DisplayConfig& config) {
    width_ = config.width;
    height_ = config.height;
    storage_ = std::make_unique<uint8_t[]>(sizeof(u8g2_t));
    handle_ = reinterpret_cast<u8g2_t*>(storage_.get());

    ESP_LOGI(kTag, "initializing u8g2 display (%s @ 0x%02x, SDA=%d, SCL=%d)",
             config.controller == DisplayController::kSsd1306 ? "ssd1306" : "sh1106",
             config.i2c_address,
             config.sda_pin,
             config.scl_pin);

    static ::u8g2_esp32_i2c_ctx_t ctx = {};
    ctx.cfg.i2c_port = config.i2c_port;
    ctx.cfg.sda_pin = config.sda_pin;
    ctx.cfg.scl_pin = config.scl_pin;
    ctx.cfg.clk_hz = 1000000;  // 1 MHz for high FPS animations
    ctx.cfg.dev_addr_7bit = config.i2c_address;
    ctx.cfg.timeout_ms = 1000;
    ctx.cfg.reset_pin = -1;
    ESP_ERROR_CHECK(u8g2_esp32_i2c_set_default_context(&ctx));
    i2c_ctx_ = &ctx;

    if (u8g2_esp32_i2c_probe(&ctx, config.i2c_address) != ESP_OK) {
        ESP_LOGW(kTag, "I2C probe failed for display at 0x%02x, retrying at 100 kHz", config.i2c_address);
        ctx.cfg.clk_hz = 100000;
        if (u8g2_esp32_i2c_probe(&ctx, config.i2c_address) != ESP_OK) {
            ESP_LOGW(kTag, "I2C probe failed for display at 0x%02x", config.i2c_address);
            return false;
        }
    }

    if (config.controller == DisplayController::kSsd1306) {
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(handle_, U8G2_R0, u8x8_byte_esp32_hw_i2c, u8x8_gpio_and_delay_esp32_i2c);
    } else {
        u8g2_Setup_sh1106_i2c_128x64_noname_f(handle_, U8G2_R0, u8x8_byte_esp32_hw_i2c, u8x8_gpio_and_delay_esp32_i2c);
    }
    u8x8_SetI2CAddress(u8g2_GetU8x8(handle_), static_cast<uint8_t>(config.i2c_address << 1));
    u8g2_InitDisplay(handle_);
    u8g2_SetPowerSave(handle_, 0);
    u8g2_SetBitmapMode(handle_, 1);
    u8g2_SetDrawColor(handle_, 1);
    set_font_small();
    clear();
    send_buffer();
    ESP_LOGI(kTag, "u8g2 display ready (%s @ 0x%02x)", config.controller == DisplayController::kSsd1306 ? "ssd1306" : "sh1106", config.i2c_address);
    return true;
}

void U8g2DisplayBackend::prepare_sleep() {
    if (i2c_ctx_ != nullptr) {
        u8g2_esp32_i2c_prepare_sleep(i2c_ctx_);
    }
}

int U8g2DisplayBackend::width() const { return width_; }
int U8g2DisplayBackend::height() const { return height_; }
void U8g2DisplayBackend::clear() { u8g2_ClearBuffer(handle_); }
void U8g2DisplayBackend::send_buffer() { u8g2_SendBuffer(handle_); }
void U8g2DisplayBackend::set_contrast(uint8_t value) { u8g2_SetContrast(handle_, value); }
void U8g2DisplayBackend::set_color(uint8_t color) { u8g2_SetDrawColor(handle_, color); }
void U8g2DisplayBackend::draw_pixel(int x, int y) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    u8g2_DrawPixel(handle_, x, y);
}
void U8g2DisplayBackend::draw_line(int x0, int y0, int x1, int y1) {
    int xmin = 0, ymin = 0, xmax = width_ - 1, ymax = height_ - 1;
    auto compute_outcode = [&](int x, int y) {
        int code = 0;
        if (x < xmin) code |= 1; else if (x > xmax) code |= 2;
        if (y < ymin) code |= 4; else if (y > ymax) code |= 8;
        return code;
    };
    int outcode0 = compute_outcode(x0, y0);
    int outcode1 = compute_outcode(x1, y1);
    bool accept = false;
    while (true) {
        if (!(outcode0 | outcode1)) { accept = true; break; }
        else if (outcode0 & outcode1) { break; }
        else {
            int x = 0, y = 0;
            int outcodeOut = outcode0 ? outcode0 : outcode1;
            if (outcodeOut & 8) { x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0); y = ymax; }
            else if (outcodeOut & 4) { x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0); y = ymin; }
            else if (outcodeOut & 2) { y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0); x = xmax; }
            else if (outcodeOut & 1) { y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0); x = xmin; }
            if (outcodeOut == outcode0) { x0 = x; y0 = y; outcode0 = compute_outcode(x0, y0); }
            else { x1 = x; y1 = y; outcode1 = compute_outcode(x1, y1); }
        }
    }
    if (accept) u8g2_DrawLine(handle_, x0, y0, x1, y1);
}
void U8g2DisplayBackend::draw_hline(int x, int y, int w) {
    if (y < 0 || y >= height_) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > width_) { w = width_ - x; }
    if (w > 0) u8g2_DrawHLine(handle_, x, y, w);
}
void U8g2DisplayBackend::draw_vline(int x, int y, int h) {
    if (x < 0 || x >= width_) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > height_) { h = height_ - y; }
    if (h > 0) u8g2_DrawVLine(handle_, x, y, h);
}
void U8g2DisplayBackend::draw_box(int x, int y, int w, int h) {
    draw_hline(x, y, w);
    draw_hline(x, y + h - 1, w);
    draw_vline(x, y, h);
    draw_vline(x + w - 1, y, h);
}
void U8g2DisplayBackend::draw_frame(int x, int y, int w, int h) {
    draw_box(x, y, w, h);
}
void U8g2DisplayBackend::draw_rbox(int x, int y, int w, int h, int r) {
    // Unsupported safe border version, fallback to fill_rbox for simplicity,
    // actually just doing fill_rbox since u8g2 draw_rbox is rarely used for lines.
    fill_rbox(x, y, w, h, r);
}
void U8g2DisplayBackend::draw_rframe(int x, int y, int w, int h, int r) {
    // Unused in our code, fallback
}
void U8g2DisplayBackend::draw_disc(int x, int y, int r) { fill_circle(x, y, r); }
void U8g2DisplayBackend::draw_circle(int x, int y, int r) {
    // Basic unclipped wrapper (unused in main engine, mostly fillCircle used)
    u8g2_DrawCircle(handle_, x, y, r, U8G2_DRAW_ALL); 
}

void U8g2DisplayBackend::fill_box(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    int x1 = x + w - 1;
    int y1 = y + h - 1;
    if (x1 < 0 || y1 < 0 || x >= width_ || y >= height_) return;
    int cx = std::max(0, x);
    int cy = std::max(0, y);
    int cw = std::min(width_ - 1, x1) - cx + 1;
    int ch = std::min(height_ - 1, y1) - cy + 1;
    if (cw > 0 && ch > 0) {
        u8g2_DrawBox(handle_, cx, cy, cw, ch);
    }
}
void U8g2DisplayBackend::fill_rbox(int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0) return;
    int max_radius = std::min(w, h) / 2;
    if (r > max_radius) r = max_radius;
    if (r <= 0) {
        fill_box(x, y, w, h);
        return;
    }
    auto draw_hline_safe = [&](int hx, int hy, int hw) {
        if (hy < 0 || hy >= height_) return;
        if (hx < 0) { hw += hx; hx = 0; }
        if (hx + hw > width_) { hw = width_ - hx; }
        if (hw > 0) u8g2_DrawHLine(handle_, hx, hy, hw);
    };
    for (int iy = y + r; iy < y + h - r; ++iy) {
        draw_hline_safe(x, iy, w);
    }
    for (int dy = 1; dy <= r; ++dy) {
        int dx = static_cast<int>(std::round(std::sqrt(r * r - dy * dy)));
        int line_width = (w - 2 * r) + 2 * dx;
        int line_x = x + r - dx;
        draw_hline_safe(line_x, y + r - dy, line_width);
        draw_hline_safe(line_x, y + h - 1 - r + dy, line_width);
    }
}
void U8g2DisplayBackend::fill_circle(int x, int y, int r) {
    if (r <= 0) return;
    auto draw_hline_safe = [&](int hx, int hy, int hw) {
        if (hy < 0 || hy >= height_) return;
        if (hx < 0) { hw += hx; hx = 0; }
        if (hx + hw > width_) { hw = width_ - hx; }
        if (hw > 0) u8g2_DrawHLine(handle_, hx, hy, hw);
    };
    draw_hline_safe(x - r, y, 2 * r + 1);
    for (int dy = 1; dy <= r; ++dy) {
        int dx = static_cast<int>(std::round(std::sqrt(r * r - dy * dy)));
        draw_hline_safe(x - dx, y - dy, 2 * dx + 1);
        draw_hline_safe(x - dx, y + dy, 2 * dx + 1);
    }
}
void U8g2DisplayBackend::fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2) {
    struct Pt {
        int x;
        int y;
    } pts[3] = {{x0, y0}, {x1, y1}, {x2, y2}};

    if (pts[1].y < pts[0].y) std::swap(pts[0], pts[1]);
    if (pts[2].y < pts[1].y) std::swap(pts[1], pts[2]);
    if (pts[1].y < pts[0].y) std::swap(pts[0], pts[1]);

    const auto draw_span = [&](int y, float xa, float xb) {
        if (y < 0 || y >= height_) {
            return;
        }
        if (xa > xb) {
            std::swap(xa, xb);
        }
        int x_start = static_cast<int>(xa + 0.5f);
        int x_end = static_cast<int>(xb + 0.5f);
        if (x_end < 0 || x_start >= width_) {
            return;
        }
        if (x_start < 0) x_start = 0;
        if (x_end >= width_) x_end = width_ - 1;
        if (x_end >= x_start) {
            u8g2_DrawHLine(handle_, x_start, y, x_end - x_start + 1);
        }
    };

    const Pt& p0 = pts[0];
    const Pt& p1 = pts[1];
    const Pt& p2 = pts[2];

    if (p0.y == p2.y) {
        draw_span(p0.y, static_cast<float>(std::min({p0.x, p1.x, p2.x})), static_cast<float>(std::max({p0.x, p1.x, p2.x})));
        return;
    }

    const auto interp_x = [](const Pt& a, const Pt& b, int y) -> float {
        if (a.y == b.y) {
            return static_cast<float>(a.x);
        }
        return static_cast<float>(a.x) + (static_cast<float>(y - a.y) * static_cast<float>(b.x - a.x)) / static_cast<float>(b.y - a.y);
    };

    for (int y = p0.y; y <= p2.y; ++y) {
        if (y < 0 || y >= height_) {
            continue;
        }

        if (y < p1.y) {
            draw_span(y, interp_x(p0, p2, y), interp_x(p0, p1, y));
        } else {
            draw_span(y, interp_x(p0, p2, y), interp_x(p1, p2, y));
        }
    }
}
void U8g2DisplayBackend::fill_round_rect(int x, int y, int w, int h, int r) { fill_rbox(x, y, w, h, r); }

void U8g2DisplayBackend::select_font(const uint8_t* font) { u8g2_SetFont(handle_, font); }
void U8g2DisplayBackend::set_font_small() { select_font(u8g2_font_profont11_tf); }
void U8g2DisplayBackend::set_font_medium() { select_font(u8g2_font_profont15_tf); }
void U8g2DisplayBackend::set_font_large() { select_font(u8g2_font_logisoso32_tn); }
void U8g2DisplayBackend::draw_text(int x, int y, const char* text) { u8g2_DrawStr(handle_, x, y, text); }
int U8g2DisplayBackend::text_width(const char* text) { return static_cast<int>(u8g2_GetStrWidth(handle_, text)); }

}  // namespace leor
