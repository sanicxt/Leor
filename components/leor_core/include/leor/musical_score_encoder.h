#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"
#include "driver/rmt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t freq_hz;
    uint32_t duration_ms;
} buzzer_musical_score_t;

typedef struct {
    uint32_t resolution;
} musical_score_encoder_config_t;

esp_err_t rmt_new_musical_score_encoder(const musical_score_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif
