#pragma once

#include <cstdint>

namespace leor {

class BuzzerService {
public:
    struct Note { uint32_t freq_hz; uint32_t dur_ms; };

    void init(int pin);
    void stop();
    void prepare_sleep();

    void play_toggle_on();
    void play_toggle_off();
    void play_power_on();
    void play_power_off();

private:
    void play_seq(const Note* notes, int count);

    int pin_ = -1;
    bool initialized_ = false;
    void* chan_ = nullptr;
    void* encoder_ = nullptr;
};

}