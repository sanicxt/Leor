#include "leor/command_router.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>

#include "esp_system.h"

namespace leor {

namespace {

std::string trim(std::string value) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool starts_with(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::vector<std::string> split(const std::string& text, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, delim)) {
        out.push_back(item);
    }
    return out;
}

}  // namespace

CommandRouter::CommandRouter(Preferences& preferences,
                             DisplayConfig& display_config,
                             DisplayBackend& display,
                             MochiEyesEngine& eyes,
                             GestureService& gestures,
                             ShuffleService& shuffle,
                             ClockService& clock,
                             PowerService& power,
                             BleService& ble)
    : preferences_(preferences),
      display_config_(display_config),
      display_(display),
      eyes_(eyes),
      gestures_(gestures),
      shuffle_(shuffle),
      clock_(clock),
      power_(power),
      ble_(ble) {}

void CommandRouter::reset_effects() {
    clock_.set_enabled(false);
    eyes_.reset_emotions();
    eyes_.set_curiosity(false);
    eyes_.set_hflicker(false, 0);
    eyes_.set_vflicker(false, 0);
    eyes_.set_sweat(false);
    eyes_.set_idle_mode(false);
    eyes_.set_eyebrows(false);
    eyes_.set_knocked(false);
    eyes_.set_position(0);
    eyes_.set_breathing(preferences_.getBool("br_en", true), eyes_.get_breathing_intensity(), eyes_.get_breathing_speed());
}

std::string CommandRouter::sync_json(uint32_t now_ms) const {
    char buf[2048];
    const unsigned ble_window_ms = static_cast<unsigned>(std::max<uint32_t>(20000U, preferences_.getUInt("ble_win", 60000)));
    std::snprintf(
        buf, sizeof(buf),
        "{\"type\":\"sync\",\"settings\":{\"ew\":%d,\"eh\":%d,\"es\":%d,\"er\":%d,\"mw\":%d,\"bi\":%d,\"gs\":%d,\"os\":%d,\"ss\":%d,\"td\":%u,\"wp\":%u,\"pp\":%u},"
        "\"display\":{\"type\":\"%s\",\"addr\":\"0x%02X\"},"
        "\"state\":{\"shuf\":%d,\"mpu\":%d,\"clk\":%d},"
        "\"clock\":{\"on\":%d,\"tz\":%d,\"sec\":%u,\"fmt\":%d},"
        "\"shuffle\":{\"emin\":%u,\"emax\":%u,\"nmin\":%u,\"nmax\":%u},"
        "\"breathing\":{\"on\":%d,\"i\":\"%.2f\",\"s\":\"%.2f\"},"
        "\"ble\":{\"win\":%u},"
        "\"gesture\":%s}",
        eyes_.eye_width(), eyes_.eye_height(), eyes_.space_between(), eyes_.border_radius(), eyes_.mouth_width(),
        static_cast<int>(preferences_.getInt("bi", 3)), static_cast<int>(preferences_.getInt("gs", 6)), static_cast<int>(preferences_.getInt("os", 12)), static_cast<int>(preferences_.getInt("ss", 10)),
        static_cast<unsigned>(power_.hold_ms()), static_cast<unsigned>(preferences_.getUInt("wake_pin", 0)), static_cast<unsigned>(preferences_.getUInt("pwr_pin", 1)),
        display_config_.controller == DisplayController::kSsd1306 ? "ssd1306" : "sh1106", display_config_.i2c_address,
        shuffle_.enabled() ? 1 : 0, mpu_verbose_ ? 1 : 0, clock_.enabled() ? 1 : 0,
        clock_.enabled() ? 1 : 0, clock_.tz_offset(), static_cast<unsigned>(clock_.seconds_of_day(now_ms)), clock_.use_24_hour() ? 24 : 12,
        static_cast<unsigned>(shuffle_.expr_min_ms() / 1000U), static_cast<unsigned>(shuffle_.expr_max_ms() / 1000U), static_cast<unsigned>(shuffle_.neutral_min_ms() / 1000U), static_cast<unsigned>(shuffle_.neutral_max_ms() / 1000U),
        eyes_.get_breathing_enabled() ? 1 : 0, eyes_.get_breathing_intensity(), eyes_.get_breathing_speed(),
        ble_window_ms, gestures_.settings_json().c_str());
    return buf;
}

std::string CommandRouter::handle_settings(const std::string& params, uint32_t now_ms) {
    if (params.empty()) {
        return sync_json(now_ms);
    }
    for (const auto& token_raw : split(params, ',')) {
        const auto token = trim(token_raw);
        const auto eq = token.find('=');
        if (eq == std::string::npos) continue;
        const auto key = token.substr(0, eq);
        const int value = std::atoi(token.substr(eq + 1).c_str());
        if (key == "ew") {
            eyes_.set_width(value, value);
            preferences_.putInt("ew", value);
        } else if (key == "eh") {
            eyes_.set_height(value, value);
            preferences_.putInt("eh", value);
        } else if (key == "es") {
            eyes_.set_space_between(value);
            preferences_.putInt("es", value);
        } else if (key == "er") {
            eyes_.set_border_radius(value, value);
            preferences_.putInt("er", value);
        } else if (key == "mw") {
            eyes_.set_mouth_size(value, 6);
            preferences_.putInt("mw", value);
        } else if (key == "td") {
            power_.set_hold_ms(value);
            preferences_.putUInt("touch_ms", power_.hold_ms());
        } else if (key == "bi") {
            eyes_.set_autoblinker(true, static_cast<float>(value), 3.0f);
            preferences_.putInt("bi", value);
        } else if (key == "gs") {
            eyes_.setGazeSpeed(static_cast<float>(value));
            preferences_.putInt("gs", value);
        } else if (key == "os") {
            eyes_.setOpennessSpeed(static_cast<float>(value));
            preferences_.putInt("os", value);
        } else if (key == "ss") {
            eyes_.setSquishSpeed(static_cast<float>(value));
            preferences_.putInt("ss", value);
        } else if (key == "wp") {
            if (value >= 0 && value <= 5) {
                preferences_.putUInt("wake_pin", static_cast<uint32_t>(value));
            }
        } else if (key == "pp") {
            if (value >= 0 && value <= 5) {
                preferences_.putUInt("pwr_pin", static_cast<uint32_t>(value));
            }
        }
    }
    return "Settings applied & saved";
}

std::string CommandRouter::handle_shuffle(const std::string& params) {
    if (params.empty()) {
        char buf[96];
        std::snprintf(buf, sizeof(buf), "Shuffle: %s\nexpr=%u-%us\nneutral=%u-%us",
                      shuffle_.enabled() ? "ON" : "OFF",
                      static_cast<unsigned>(shuffle_.expr_min_ms() / 1000U), static_cast<unsigned>(shuffle_.expr_max_ms() / 1000U),
                      static_cast<unsigned>(shuffle_.neutral_min_ms() / 1000U), static_cast<unsigned>(shuffle_.neutral_max_ms() / 1000U));
        return buf;
    }
    for (const auto& token_raw : split(params, ',')) {
        const auto token = trim(token_raw);
        if (token == "on" || token == "1") {
            shuffle_.set_enabled(true);
        } else if (token == "off" || token == "0") {
            shuffle_.set_enabled(false);
        } else if (token == "quick") {
            shuffle_.set_expr_range(1000, 2000);
            shuffle_.set_neutral_range(500, 1500);
            shuffle_.set_enabled(true);
        } else if (token == "slow") {
            shuffle_.set_expr_range(4000, 8000);
            shuffle_.set_neutral_range(3000, 6000);
            shuffle_.set_enabled(true);
        } else if (starts_with(token, "expr=") || starts_with(token, "e=")) {
            const auto eq = token.find('=');
            const std::string value = token.substr(eq + 1);
            const auto dash = value.find('-');
            uint32_t min_s = 1;
            uint32_t max_s = 1;
            if (dash != std::string::npos) {
                min_s = static_cast<uint32_t>(std::max(1, std::atoi(value.substr(0, dash).c_str())));
                max_s = static_cast<uint32_t>(std::max(static_cast<int>(min_s), std::atoi(value.substr(dash + 1).c_str())));
            } else {
                min_s = static_cast<uint32_t>(std::max(1, std::atoi(value.c_str())));
                max_s = min_s;
            }
            shuffle_.set_expr_range(min_s * 1000U, max_s * 1000U);
        } else if (starts_with(token, "neutral=") || starts_with(token, "n=")) {
            const auto eq = token.find('=');
            const std::string value = token.substr(eq + 1);
            const auto dash = value.find('-');
            uint32_t min_s = 1;
            uint32_t max_s = 1;
            if (dash != std::string::npos) {
                min_s = static_cast<uint32_t>(std::max(1, std::atoi(value.substr(0, dash).c_str())));
                max_s = static_cast<uint32_t>(std::max(static_cast<int>(min_s), std::atoi(value.substr(dash + 1).c_str())));
            } else {
                min_s = static_cast<uint32_t>(std::max(1, std::atoi(value.c_str())));
                max_s = min_s;
            }
            shuffle_.set_neutral_range(min_s * 1000U, max_s * 1000U);
        }
    }
    preferences_.putBool("shuf_en", shuffle_.enabled());
    preferences_.putUInt("shuf_emin", shuffle_.expr_min_ms());
    preferences_.putUInt("shuf_emax", shuffle_.expr_max_ms());
    preferences_.putUInt("shuf_nmin", shuffle_.neutral_min_ms());
    preferences_.putUInt("shuf_nmax", shuffle_.neutral_max_ms());
    return handle_shuffle("");
}

std::string CommandRouter::handle_display(const std::string& params) {
    if (starts_with(params, "type=")) {
        const auto type = lower(trim(params.substr(5)));
        if (type == "ssd1306") {
            preferences_.putString("disp_type", type);
            display_config_.controller = DisplayController::kSsd1306;
            return "display:type=ssd1306 saved. Restart required: send 'restart' command";
        }
        if (type == "sh1106") {
            preferences_.putString("disp_type", type);
            display_config_.controller = DisplayController::kSh1106;
            return "display:type=sh1106 saved. Restart required: send 'restart' command";
        }
        return "display:type invalid. Use: sh1106 or ssd1306";
    }
    if (starts_with(params, "addr=")) {
        const auto raw = trim(params.substr(5));
        const long value = std::strtol(raw.c_str(), nullptr, 16);
        if (value >= 0 && value <= 0x7f) {
            display_config_.i2c_address = static_cast<uint8_t>(value);
            preferences_.putUInt("disp_addr", value);
            return "display:addr=" + raw + " saved. Restart required: send 'restart' command";
        }
        return "display:addr invalid. Use hex format: 0x3C or 0x3D";
    }
    if (params == "test") {
        display_.clear();
        display_.set_font_medium();
        display_.draw_text(20, 26, "DISPLAY");
        display_.draw_text(35, 48, "TEST");
        display_.send_buffer();
        return "display:test complete";
    }
    if (params == "clear") {
        display_.clear();
        display_.send_buffer();
        return "display:clear";
    }
    if (params == "info") {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Display: %s @ 0x%02X (%dx%d)", display_config_.controller == DisplayController::kSsd1306 ? "SSD1306" : "SH1106", display_config_.i2c_address, display_.width(), display_.height());
        return buf;
    }
    return "display: usage - type=<sh1106|ssd1306>, addr=<hex>, test, clear, info";
}

std::string CommandRouter::handle_clock(const std::string& params, uint32_t now_ms) {
    if (params.empty()) return clock_.status_string(now_ms, ble_.connected());
    if (params == "on") {
        clock_.set_enabled(true);
        preferences_.putBool("clk_on", true);
        return clock_.status_string(now_ms, ble_.connected());
    }
    if (params == "off") {
        clock_.set_enabled(false);
        preferences_.putBool("clk_on", false);
        return clock_.status_string(now_ms, ble_.connected());
    }
    if (starts_with(params, "fmt=")) {
        const int fmt = std::atoi(params.substr(4).c_str());
        if (fmt == 12 || fmt == 24) {
            clock_.set_use_24_hour(fmt == 24);
            preferences_.putBool("clk_24", fmt == 24);
            return clock_.status_string(now_ms, ble_.connected());
        }
        return "clock:fmt invalid. Use 12 or 24";
    }
    if (starts_with(params, "sync=")) {
        const auto value = params.substr(5);
        const auto comma = value.find(',');
        const uint64_t epoch = std::strtoull(value.substr(0, comma).c_str(), nullptr, 10);
        const int16_t tz = comma == std::string::npos ? 0 : static_cast<int16_t>(std::atoi(value.substr(comma + 1).c_str()));
        clock_.set_from_epoch_ms(epoch, tz, now_ms);
        preferences_.putULong64("clk_epoch", epoch);
        preferences_.putInt("clk_tz", tz);
        preferences_.putUInt("clk_sec", clock_.seconds_of_day(now_ms));
        return clock_.status_string(now_ms, ble_.connected());
    }
    if (starts_with(params, "set=")) {
        const auto parts = split(params.substr(4), ':');
        if (parts.size() < 2) return "clock:set invalid. Use HH:MM or HH:MM:SS";
        const int hh = std::atoi(parts[0].c_str());
        const int mm = std::atoi(parts[1].c_str());
        const int ss = parts.size() > 2 ? std::atoi(parts[2].c_str()) : 0;
        if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) return "clock:set invalid. Use HH:MM or HH:MM:SS";
        clock_.set_time_of_day(static_cast<uint8_t>(hh), static_cast<uint8_t>(mm), static_cast<uint8_t>(ss), now_ms);
        preferences_.putUInt("clk_sec", hh * 3600U + mm * 60U + ss);
        preferences_.putULong64("clk_epoch", 0);
        return clock_.status_string(now_ms, ble_.connected());
    }
    return "clock: usage - on, off, set=HH:MM[:SS], sync=EPOCH_MS[,TZ], fmt=12|24";
}

std::string CommandRouter::handle(std::string cmd, uint32_t now_ms, bool is_manual) {
    cmd = trim(cmd);
    if (cmd.empty()) return "Empty command";
    cmd = lower(cmd);

    auto set_expression = [&](int mood, int gaze, int mouth_type) {
        reset_effects();
        eyes_.set_mood(mood);
        eyes_.set_position(gaze);
        eyes_.set_mouth_type(mouth_type);
        if (is_manual) shuffle_.reset();
    };

    if (cmd == "happy") { set_expression(HAPPY, 0, 1); eyes_.setExpression(EXPR_HAPPY); return "Expression: Happy"; }
    if (cmd == "sad") { set_expression(TIRED, 0, 2); eyes_.setExpression(EXPR_SAD); return "Expression: Sad"; }
    if (cmd == "angry") { set_expression(ANGRY, 0, 5); eyes_.setExpression(EXPR_ANGRY); return "Expression: Angry"; }
    if (cmd == "love") { set_expression(DEFAULT, 0, 3); eyes_.setExpression(EXPR_NORMAL); eyes_.anim_love(); return "Expression: Love"; }
    if (cmd == "surprised") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_SURPRISED); eyes_.blink(); return "Expression: Surprised"; }
    if (cmd == "confused") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_NORMAL); eyes_.anim_confused(); return "Expression: Confused"; }
    if (cmd == "sleepy") { set_expression(TIRED, POS_SW, 5); eyes_.setExpression(EXPR_SLEEPY); return "Expression: Sleepy"; }
    if (cmd == "curious") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_NORMAL); eyes_.set_curiosity(true); return "Expression: Curious"; }
    if (cmd == "nervous") { set_expression(DEFAULT, 0, 9); eyes_.setExpression(EXPR_WORRIED); eyes_.set_sweat(true); eyes_.set_curiosity(true); return "Expression: Nervous"; }
    if (cmd == "knocked" || cmd == "dizzy") { reset_effects(); eyes_.set_knocked(true); return "Expression: Knocked"; }
    if (cmd == "neutral" || cmd == "normal" || cmd == "reset") { set_expression(DEFAULT, 0, 1); eyes_.setExpression(EXPR_NORMAL); return "Expression: Neutral"; }
    if (cmd == "idle") { set_expression(DEFAULT, 0, 1); eyes_.setExpression(EXPR_NORMAL); eyes_.set_idle_mode(true, 1, 2); return "Mode: Idle"; }
    if (cmd == "raised") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_NORMAL); eyes_.set_eyebrows(true); return "Expression: Raised eyebrows"; }
    // New esp32-eyes expressions
    if (cmd == "glee") { set_expression(HAPPY, 0, 1); eyes_.setExpression(EXPR_GLEE); return "Expression: Glee"; }
    if (cmd == "worried") { set_expression(DEFAULT, 0, 9); eyes_.setExpression(EXPR_WORRIED); return "Expression: Worried"; }
    if (cmd == "focused") { set_expression(DEFAULT, 0, 5); eyes_.setExpression(EXPR_FOCUSED); return "Expression: Focused"; }
    if (cmd == "annoyed") { set_expression(DEFAULT, 0, 9); eyes_.setExpression(EXPR_ANNOYED); return "Expression: Annoyed"; }
    if (cmd == "skeptic") { set_expression(DEFAULT, 0, 2); eyes_.setExpression(EXPR_SKEPTIC); return "Expression: Skeptic"; }
    if (cmd == "frustrated") { set_expression(DEFAULT, 0, 9); eyes_.setExpression(EXPR_FRUSTRATED); return "Expression: Frustrated"; }
    if (cmd == "unimpressed") { set_expression(DEFAULT, 0, 2); eyes_.setExpression(EXPR_UNIMPRESSED); return "Expression: Unimpressed"; }
    if (cmd == "suspicious") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_SUSPICIOUS); return "Expression: Suspicious"; }
    if (cmd == "squint") { set_expression(DEFAULT, 0, 4); eyes_.setExpression(EXPR_SQUINT); return "Expression: Squint"; }
    if (cmd == "furious") { set_expression(ANGRY, 0, 9); eyes_.setExpression(EXPR_FURIOUS); return "Expression: Furious"; }
    if (cmd == "scared") { set_expression(DEFAULT, 0, 10); eyes_.setExpression(EXPR_SCARED); return "Expression: Scared"; }
    if (cmd == "awe") { set_expression(DEFAULT, 0, 10); eyes_.setExpression(EXPR_AWE); return "Expression: Awe"; }
    auto set_mouth = [&](int type, const char* name) {
        eyes_.set_mouth_type(type);
        if (is_manual) shuffle_.reset();
        return std::string("Mouth: ") + name;
    };
    auto set_pos = [&](int pos, const char* name) {
        eyes_.set_position(pos);
        if (is_manual) shuffle_.reset();
        return std::string("Position: ") + name;
    };
    auto do_action = [&](auto func, const char* name) {
        func();
        if (is_manual) shuffle_.reset();
        return std::string("Action: ") + name;
    };

    if (cmd == "smile") return set_mouth(1, "Smile");
    if (cmd == "frown") return set_mouth(2, "Frown");
    if (cmd == "open") return set_mouth(3, "Open");
    if (cmd == "ooo") return set_mouth(4, "Ooo");
    if (cmd == "flat") return set_mouth(5, "Flat");
    if (cmd == "uwu") return do_action([&]{ eyes_.trigger_uwu(); }, "UwU");
    if (cmd == "xd") return do_action([&]{ eyes_.trigger_xd(); }, "XD");
    if (cmd == "uwum") return set_mouth(6, "UwU");
    if (cmd == "xdm") return set_mouth(7, "XD");
    if (cmd == "smirk") return set_mouth(8, "Smirk");
    if (cmd == "zigzag") return set_mouth(9, "Zigzag");
    if (cmd == "bigo") return set_mouth(10, "Big O");
    if (starts_with(cmd, "talk")) {
        uint32_t duration = 3000;
        const auto pos = cmd.find(' ');
        if (pos != std::string::npos) duration = static_cast<uint32_t>(std::max(100, std::atoi(cmd.substr(pos + 1).c_str())));
        eyes_.start_mouth_anim(1, duration);
        return "Mouth: Talking";
    }
    if (starts_with(cmd, "chew")) {
        uint32_t duration = 2000;
        const auto pos = cmd.find(' ');
        if (pos != std::string::npos) duration = static_cast<uint32_t>(std::max(100, std::atoi(cmd.substr(pos + 1).c_str())));
        eyes_.start_mouth_anim(2, duration);
        return "Mouth: Chewing";
    }
    if (starts_with(cmd, "wobble")) {
        uint32_t duration = 2000;
        const auto pos = cmd.find(' ');
        if (pos != std::string::npos) duration = static_cast<uint32_t>(std::max(100, std::atoi(cmd.substr(pos + 1).c_str())));
        eyes_.start_mouth_anim(3, duration);
        return "Mouth: Wobbling";
    }

    if (cmd == "blink") return do_action([&]{ eyes_.blink(); }, "Blink");
    if (cmd == "wink") return do_action([&]{ eyes_.wink(true); eyes_.set_mouth_type(1); }, "Wink");
    if (cmd == "winkr") return do_action([&]{ eyes_.wink(false); eyes_.set_mouth_type(1); }, "Wink Right");
    if (cmd == "laugh") return do_action([&]{ eyes_.anim_laugh(); }, "Laugh");
    if (cmd == "cry") return do_action([&]{ eyes_.anim_cry(); }, "Cry");

    if (cmd == "center") return set_pos(0, "Center");
    if (cmd == "n" || cmd == "up") return set_pos(POS_N, "North");
    if (cmd == "ne") return set_pos(POS_NE, "North-East");
    if (cmd == "e" || cmd == "right") return set_pos(POS_E, "East");
    if (cmd == "se") return set_pos(POS_SE, "South-East");
    if (cmd == "s" || cmd == "down") return set_pos(POS_S, "South");
    if (cmd == "sw") return set_pos(POS_SW, "South-West");
    if (cmd == "w" || cmd == "left") return set_pos(POS_W, "West");
    if (cmd == "nw") return set_pos(POS_NW, "North-West");

    if (cmd == "sweat") { eyes_.set_sweat(true); return "Sweat: ON"; }
    if (cmd == "cyclops") { eyes_.set_cyclops(true); return "Cyclops: ON"; }
    if (cmd == "br:") {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "br:%s i=%.2f s=%.2f", eyes_.get_breathing_enabled() ? "on" : "off", eyes_.get_breathing_intensity(), eyes_.get_breathing_speed());
        return buf;
    }
    if (starts_with(cmd, "br=")) { const bool on = std::atoi(cmd.substr(3).c_str()) == 1; eyes_.set_breathing(on); preferences_.putBool("br_en", on); return on ? "Breathing: ON" : "Breathing: OFF"; }
    if (cmd == "br") { const bool on = !eyes_.get_breathing_enabled(); eyes_.set_breathing(on); preferences_.putBool("br_en", on); return on ? "Breathing: ON" : "Breathing: OFF"; }
    if (starts_with(cmd, "bri=")) { const float value = std::strtof(cmd.substr(4).c_str(), nullptr); eyes_.set_breathing_intensity(value); preferences_.putFloat("br_int", value); return "Breathing intensity updated"; }
    if (starts_with(cmd, "brs=")) { const float value = std::strtof(cmd.substr(4).c_str(), nullptr); eyes_.set_breathing_speed(value); preferences_.putFloat("br_spd", value); return "Breathing speed updated"; }
    if (cmd == "mouth") { eyes_.set_mouth_enabled(false); return "Mouth toggled"; }
    if (cmd == "mpulog") { mpu_verbose_ = !mpu_verbose_; return mpu_verbose_ ? "MPU verbose ON" : "MPU verbose OFF"; }

    if (starts_with(cmd, "s:") || starts_with(cmd, "set:")) return handle_settings(cmd.substr(cmd[1] == ':' ? 2 : 4), now_ms);
    if (cmd == "gs" || cmd == "gx") return cmd == "gs" ? "gs:1" : "gs:0";
    if (starts_with(cmd, "ga=")) { const auto params = cmd.substr(3); const auto pos = params.find(':'); if (pos != std::string::npos) { gestures_.set_action(std::atoi(params.substr(0, pos).c_str()), trim(params.substr(pos + 1))); return "ga:ok"; } return "ga:err"; }
    if (starts_with(cmd, "gm=")) { const bool on = std::atoi(cmd.substr(3).c_str()) == 1; gestures_.set_matching_enabled(on); return on ? "gm=1" : "gm=0"; }
    if (cmd == "gc") return "gc:ok";
    if (cmd == "gi") return gestures_.list_json();
    if (cmd == "gs:") return gestures_.settings_json();
    if (starts_with(cmd, "grt=")) { gestures_.set_reaction_time(std::atoi(cmd.substr(4).c_str())); return "rt=" + std::to_string(gestures_.reaction_time_ms()); }
    if (starts_with(cmd, "gcf=")) { gestures_.set_confidence(std::atoi(cmd.substr(4).c_str())); return "cf=" + std::to_string(gestures_.confidence_percent()); }
    if (starts_with(cmd, "gcd=")) { gestures_.set_cooldown(std::atoi(cmd.substr(4).c_str())); return "cd=" + std::to_string(gestures_.cooldown_ms()); }

    if (cmd == "ble:") return std::string("ble:win=") + std::to_string(std::max<uint32_t>(20000U, preferences_.getUInt("ble_win", 60000)));
    if (starts_with(cmd, "ble:win=")) {
        const int window_ms = std::atoi(cmd.substr(8).c_str());
        const int clamped_ms = std::max(20000, std::min(window_ms, 600000));
        preferences_.putUInt("ble_win", static_cast<uint32_t>(clamped_ms));
        return "ble:win=" + std::to_string(clamped_ms);
    }
    if (cmd == "ble:name") return "ble:name=" + preferences_.getString("ble_name", "Leor");
    if (starts_with(cmd, "ble:name=")) {
        const auto name = trim(cmd.substr(9));
        preferences_.putString("ble_name", name);
        return "ble:name=" + name + " saved. Reconnect now; restart if not visible.";
    }
    if (cmd == "tw:") return "tw:pin=" + std::to_string(preferences_.getUInt("wake_pin", 0)) + " active=high hold=" + std::to_string(power_.hold_ms()) + "ms";
    if (starts_with(cmd, "sh:") || starts_with(cmd, "shuffle:")) return handle_shuffle(cmd.substr(cmd[2] == ':' ? 3 : 8));
    if (starts_with(cmd, "display:")) return handle_display(trim(cmd.substr(8)));
    if (starts_with(cmd, "clock:")) return handle_clock(trim(cmd.substr(6)), now_ms);
    if (cmd == "restart" || cmd == "reboot") { esp_restart(); return "Restarting..."; }
    if (cmd == "help" || cmd == "?") return "help";
    return "Unknown: " + cmd;
}

}  // namespace leor
