#include "leor/notification_overlay.hpp"
#include "leor/display_backend.hpp"
#include "leor/icons/icons.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace leor {

void NotificationOverlay::show_msg(const char* t, const char* b, const uint8_t* ic, uint32_t now, uint32_t dur) {
    type = NotificationType::kMessage;
    std::strncpy(title, t, sizeof(title) - 1);
    std::strncpy(body, b, sizeof(body) - 1);
    std::memset(sub, 0, sizeof(sub));
    std::memset(location, 0, sizeof(location));
    icon = ic;
    started_ms = now;
    duration_ms = dur;
    active = true;
}

void NotificationOverlay::show_call(const char* caller, const uint8_t* ic, uint32_t now) {
    type = NotificationType::kCall;
    std::strncpy(title, caller, sizeof(title) - 1);
    std::memset(body, 0, sizeof(body));
    std::memset(sub, 0, sizeof(sub));
    std::memset(location, 0, sizeof(location));
    icon = ic;
    started_ms = now;
    duration_ms = 0;
    active = true;
}

void NotificationOverlay::show_cal(const char* t, const char* time_str, const char* loc, const uint8_t* ic, uint32_t now, uint32_t dur) {
    type = NotificationType::kCalendar;
    std::strncpy(title, t, sizeof(title) - 1);
    std::strncpy(body, time_str, sizeof(body) - 1);
    std::strncpy(location, loc, sizeof(location) - 1);
    std::memset(sub, 0, sizeof(sub));
    icon = ic;
    started_ms = now;
    duration_ms = dur;
    active = true;
}

void NotificationOverlay::dismiss() {
    active = false;
    type = NotificationType::kMessage;
}

bool NotificationOverlay::expired(uint32_t now) const {
    if (type == NotificationType::kCall) return false;
    return duration_ms > 0 && (now - started_ms) >= duration_ms;
}

const uint8_t* icon_for_app(const char* app_name) {
    std::string name = app_name;
    for (auto& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (name == "whatsapp") return leor::icons::whatsapp;
    if (name == "discord") return leor::icons::discord;
    if (name == "phone") return leor::icons::phone;
    if (name == "telegram") return leor::icons::telegram;
    if (name == "instagram") return leor::icons::instagram;
    if (name == "email" || name == "mail") return leor::icons::email;
    if (name == "facebook") return leor::icons::facebook;
    if (name == "messenger") return leor::icons::facebook_messenger;
    if (name == "spotify") return leor::icons::spotify;
    if (name == "github") return leor::icons::github;
    if (name == "linkedin") return leor::icons::linkedin;
    if (name == "youtube") return leor::icons::youtube;
    if (name == "tiktok") return leor::icons::tiktok;
    if (name == "twitter" || name == "x") return leor::icons::twitter;
    if (name == "reddit") return leor::icons::reddit;
    if (name == "notion") return leor::icons::notion;
    if (name == "mochi") return leor::icons::mochi;
    if (name == "clock" || name == "alarm") return leor::icons::clock;
    if (name == "calendar" || name == "cal" || name == "event") return leor::icons::calendar;
    if (name == "settings") return leor::icons::settings;
    if (name == "sleep") return leor::icons::sleep;
    return leor::icons::notification;
}

namespace {

void draw_header(DisplayBackend& disp, const char* label, const uint8_t* icon, uint32_t now) {
    disp.set_color(1);
    disp.fill_box(0, 0, 128, 16);
    disp.set_color(0);

    disp.draw_xbmp(2, 0, 16, 16, icon);

    disp.set_font_small();
    disp.draw_text(20, 11, label);

    uint32_t secs = (now / 1000) % 86400;
    uint8_t hh = static_cast<uint8_t>(secs / 3600);
    uint8_t mm = static_cast<uint8_t>((secs % 3600) / 60);
    char time_buf[8];
    std::snprintf(time_buf, sizeof(time_buf), "%02u:%02u", hh, mm);
    int tw = disp.text_width(time_buf);
    disp.draw_text(128 - tw - 2, 11, time_buf);

    disp.set_color(1);
}

void wrap_text_word(const char* text, int max_width, DisplayBackend& disp,
                    char lines[3][64], int& line_count) {
    line_count = 0;
    std::string remaining = text;

    while (!remaining.empty() && line_count < 3) {
        if (disp.text_width(remaining.c_str()) <= max_width) {
            std::strncpy(lines[line_count], remaining.c_str(), 63);
            lines[line_count][63] = '\0';
            line_count++;
            break;
        }

        int last_space = 0;
        int best = 0;
        for (size_t i = 1; i <= remaining.size(); ++i) {
            if (remaining[i - 1] == ' ') last_space = static_cast<int>(i);
            std::string sub = remaining.substr(0, i);
            if (disp.text_width(sub.c_str()) <= max_width) {
                best = static_cast<int>(i);
            } else {
                break;
            }
        }

        int break_at = (last_space > 0) ? last_space : best;
        if (break_at == 0) break_at = 1;

        std::strncpy(lines[line_count], remaining.substr(0, break_at).c_str(), 63);
        lines[line_count][63] = '\0';
        line_count++;

        size_t next = static_cast<size_t>(break_at);
        while (next < remaining.size() && remaining[next] == ' ') next++;
        remaining = remaining.substr(next);
    }
}

}  // namespace

// ── Message Notification ─────────────────────────────────────────────────────

static void draw_message_overlay(DisplayBackend& disp, const NotificationOverlay& n, uint32_t now) {
    draw_header(disp, n.title, leor::icons::notification, now);

    disp.draw_hline(0, 16, 128);

    disp.draw_xbmp(4, 22, 16, 16, n.icon);

    disp.set_font_medium();
    int max_w = 128 - 24;

    char lines[3][64] = {};
    int line_count = 0;
    wrap_text_word(n.body, max_w, disp, lines, line_count);

    // Ellipsis on last line if text was truncated
    if (line_count > 0) {
        size_t total_written = 0;
        for (int i = 0; i < line_count; ++i) total_written += std::strlen(lines[i]);
        if (total_written < std::strlen(n.body)) {
            int last = std::min(line_count, 3) - 1;
            std::string last_line = lines[last];
            int last_max = (last < 2) ? max_w : (128 - 24);
            std::string with_dots = last_line + "...";
            while (!last_line.empty() && disp.text_width(with_dots.c_str()) > last_max) {
                last_line.pop_back();
                with_dots = last_line + "...";
            }
            if (!last_line.empty()) {
                std::strncpy(lines[last], with_dots.c_str(), 63);
                lines[last][63] = '\0';
            }
        }
    }

    for (int i = 0; i < line_count && i < 2; ++i) {
        disp.draw_text(22, 35 + i * 14, lines[i]);
    }
    if (line_count > 2) {
        disp.set_font_small();
        disp.draw_text(22, 61, lines[2]);
    }
}

// ── Call Screen ──────────────────────────────────────────────────────────────

static void draw_call_screen(DisplayBackend& disp, const NotificationOverlay& n, uint32_t now) {
    disp.set_color(1);
    disp.fill_box(0, 0, 128, 64);
    disp.set_color(0);

    disp.draw_xbmp(56, 8, 16, 16, n.icon);

    disp.set_font_medium();
    int tw = disp.text_width(n.title);
    if (tw > 120) {
        std::string name = n.title;
        std::string with_dots = name + "...";
        while (!name.empty() && disp.text_width(with_dots.c_str()) > 120) {
            name.pop_back();
            with_dots = name + "...";
        }
        disp.draw_text(4, 40, with_dots.c_str());
    } else {
        disp.draw_text((128 - tw) / 2, 40, n.title);
    }

    disp.set_font_small();
    tw = disp.text_width("Incoming Call");
    disp.draw_text((128 - tw) / 2, 54, "Incoming Call");

    disp.set_color(1);
}

// ── Calendar Screen ──────────────────────────────────────────────────────────

static void draw_calendar_screen(DisplayBackend& disp, const NotificationOverlay& n, uint32_t now) {
    draw_header(disp, "Calendar", leor::icons::calendar, now);
    disp.draw_hline(0, 16, 128);

    disp.set_font_medium();
    int tw = disp.text_width(n.title);
    disp.draw_text((128 - tw) / 2, 28, n.title);

    if (std::strlen(n.body) > 0) {
        disp.set_font_small();
        tw = disp.text_width(n.body);
        disp.draw_text((128 - tw) / 2, 48, n.body);
    }

    if (std::strlen(n.location) > 0) {
        tw = disp.text_width(n.location);
        disp.draw_text((128 - tw) / 2, 58, n.location);
    }
}

// ── Dispatch ─────────────────────────────────────────────────────────────────

void draw_notification(DisplayBackend& disp, const NotificationOverlay& n, uint32_t now) {
    switch (n.type) {
    case NotificationType::kMessage:
        draw_message_overlay(disp, n, now);
        break;
    case NotificationType::kCall:
        draw_call_screen(disp, n, now);
        break;
    case NotificationType::kCalendar:
        draw_calendar_screen(disp, n, now);
        break;
    }
}

}  // namespace leor
