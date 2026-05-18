#include "leor/buzzer_service.hpp"
#include "leor/musical_score_encoder.h"

#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"

namespace leor {

static const char* kTag = "leor_buzzer";
static constexpr uint32_t kRes = 1000000;

static const BuzzerService::Note kToggleOn[]  = {{2800, 60}};
static const BuzzerService::Note kToggleOff[] = {{1800, 60}};
static const BuzzerService::Note kPowerOn[]   = {{2800, 70}, {3500, 70}, {4200, 100}};
static const BuzzerService::Note kPowerOff[]  = {{2800, 100}, {1800, 150}};

void BuzzerService::init(int pin) {
    if (pin < 0) { ESP_LOGW(kTag, "no pin, disabled"); return; }
    pin_ = pin;

    gpio_num_t g = static_cast<gpio_num_t>(pin_);
    esp_rom_gpio_pad_select_gpio(g);
    gpio_set_drive_capability(g, GPIO_DRIVE_CAP_3);

    rmt_tx_channel_config_t tx = {};
    tx.clk_src = RMT_CLK_SRC_DEFAULT;
    tx.gpio_num = g;
    tx.mem_block_symbols = 64;
    tx.resolution_hz = kRes;
    tx.trans_queue_depth = 10;

    rmt_channel_handle_t ch = nullptr;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx, &ch));
    chan_ = ch;

    musical_score_encoder_config_t ec = {.resolution = kRes};
    rmt_encoder_handle_t enc = nullptr;
    ESP_ERROR_CHECK(rmt_new_musical_score_encoder(&ec, &enc));
    encoder_ = enc;

    ESP_ERROR_CHECK(rmt_enable(ch));
    initialized_ = true;
    ESP_LOGI(kTag, "buzzer GPIO%d (RMT, 40mA drive)", pin_);
}

void BuzzerService::play_seq(const Note* notes, int count) {
    if (!initialized_ || count < 1) return;
    auto* ch  = static_cast<rmt_channel_handle_t>(chan_);
    auto* enc = static_cast<rmt_encoder_handle_t>(encoder_);
    rmt_encoder_reset(enc);

    for (int i = 0; i < count; ++i) {
        buzzer_musical_score_t s = {notes[i].freq_hz, notes[i].dur_ms};
        rmt_transmit_config_t tc = {};
        uint32_t cycles = s.duration_ms * s.freq_hz / 1000;
        tc.loop_count = cycles > 0 ? cycles : 1;
        ESP_ERROR_CHECK(rmt_transmit(ch, enc, &s, sizeof(s), &tc));
    }

    // Wait for all queued transmissions to finish before touching the channel.
    // rmt_disable mid-transmission leaves the RMT in a glitch state that
    // produces a saw-like residual waveform on the pin.
    rmt_tx_wait_all_done(ch, -1);
    rmt_disable(ch);
    gpio_set_level(static_cast<gpio_num_t>(pin_), 0);
    rmt_enable(ch);
}

void BuzzerService::stop() {
    if (!initialized_) return;
    auto* ch = static_cast<rmt_channel_handle_t>(chan_);
    auto* enc = static_cast<rmt_encoder_handle_t>(encoder_);
    rmt_tx_wait_all_done(ch, -1);
    rmt_disable(ch);
    rmt_encoder_reset(enc);
    rmt_enable(ch);
}

void BuzzerService::play_toggle_on()  { play_seq(kToggleOn, 1); }
void BuzzerService::play_toggle_off() { play_seq(kToggleOff, 1); }
void BuzzerService::play_power_on()   { play_seq(kPowerOn, 3); }
void BuzzerService::play_power_off()  { play_seq(kPowerOff, 2); }

void BuzzerService::prepare_sleep() {
    if (!initialized_ || pin_ < 0) return;
    stop();
    gpio_num_t g = static_cast<gpio_num_t>(pin_);
    esp_rom_gpio_pad_select_gpio(g);
    gpio_hold_dis(g);
    gpio_config_t c = {};
    c.pin_bit_mask = (1ULL << g);
    c.mode = GPIO_MODE_OUTPUT;
    c.pull_up_en = GPIO_PULLUP_DISABLE;
    c.pull_down_en = GPIO_PULLDOWN_DISABLE;
    c.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&c);
    gpio_set_level(g, 0);
    gpio_hold_en(g);
}

}