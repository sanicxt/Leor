#pragma once

#include <cstdint>
#include <string>

namespace leor {

enum class HwState : uint8_t {
    kPresent = 0,
    kAbsent = 1,
    kProbeFailed = 2,
};

struct HardwareStatus {
    HwState display = HwState::kPresent;
    HwState gyro = HwState::kPresent;
    HwState buzzer = HwState::kPresent;
    HwState touch = HwState::kPresent;
    HwState power = HwState::kPresent;

    bool any_failure() const {
        return display == HwState::kProbeFailed ||
               gyro == HwState::kProbeFailed ||
               buzzer == HwState::kAbsent ||
               touch == HwState::kAbsent ||
               power == HwState::kAbsent;
    }

    // Returns e.g. "!GYRO !BUZZ" (space-separated, empty string if all OK).
    std::string summary() const {
        std::string out;
        auto add = [&](const char* tag) {
            if (!out.empty()) out += ' ';
            out += '!';
            out += tag;
        };
        if (display == HwState::kProbeFailed) add("DISP");
        if (gyro == HwState::kProbeFailed) add("GYRO");
        if (buzzer == HwState::kAbsent) add("BUZZ");
        if (touch == HwState::kAbsent) add("TOUCH");
        if (power == HwState::kAbsent) add("PWR");
        return out;
    }
};

} // namespace leor
