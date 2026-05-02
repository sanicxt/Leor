#pragma once

#include <cstdint>
#include <cstring>

namespace leor {

class DisplayBackend;

enum class NotificationType : uint8_t {
    kMessage,
    kCall,
    kCalendar,
};

struct NotificationOverlay {
    NotificationType type = NotificationType::kMessage;
    char title[32] = {};
    char body[128] = {};
    char sub[64] = {};
    char location[64] = {};
    const uint8_t* icon = nullptr;
    uint32_t started_ms = 0;
    uint32_t duration_ms = 5000;
    bool active = false;

    void show_msg(const char* title, const char* body, const uint8_t* ic, uint32_t now, uint32_t dur);
    void show_call(const char* caller, const uint8_t* ic, uint32_t now);
    void show_cal(const char* title, const char* time, const char* loc, const uint8_t* ic, uint32_t now, uint32_t dur);
    void dismiss();
    bool expired(uint32_t now) const;
};

void draw_notification(DisplayBackend& disp, const NotificationOverlay& n, uint32_t now);

const uint8_t* icon_for_app(const char* app_name);

}  // namespace leor
