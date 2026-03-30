#include "leor/shuffle_service.hpp"

#include <cstdlib>

namespace leor {

void ShuffleService::restore(bool enabled, uint32_t expr_min_ms, uint32_t expr_max_ms, uint32_t neutral_min_ms, uint32_t neutral_max_ms) {
    enabled_ = enabled;
    expr_min_ms_ = expr_min_ms;
    expr_max_ms_ = expr_max_ms;
    neutral_min_ms_ = neutral_min_ms;
    neutral_max_ms_ = neutral_max_ms;
    reset();
}

void ShuffleService::reset() {
    needs_init_ = true;
    expression_phase_ = false;
    next_change_ms_ = 0;
    last_shuffle_index_ = -1;
}

void ShuffleService::set_enabled(bool enabled) {
    enabled_ = enabled;
    if (enabled_) {
        reset();
    }
}

void ShuffleService::set_expr_range(uint32_t min_ms, uint32_t max_ms) {
    expr_min_ms_ = min_ms;
    expr_max_ms_ = max_ms < min_ms ? min_ms : max_ms;
}

void ShuffleService::set_neutral_range(uint32_t min_ms, uint32_t max_ms) {
    neutral_min_ms_ = min_ms;
    neutral_max_ms_ = max_ms < min_ms ? min_ms : max_ms;
}

bool ShuffleService::should_emit(uint32_t now_ms, bool reacting, bool training, const char** command_out) {
    if (!enabled_ || reacting || training) {
        return false;
    }
    if (needs_init_) {
        needs_init_ = false;
        expression_phase_ = false;
        next_change_ms_ = now_ms + neutral_min_ms_ + (std::rand() % (neutral_max_ms_ - neutral_min_ms_ + 1U));
        return false;
    }
    if (next_change_ms_ != 0 && now_ms < next_change_ms_) {
        return false;
    }
    if (expression_phase_) {
        expression_phase_ = false;
        next_change_ms_ = now_ms + neutral_min_ms_ + (std::rand() % (neutral_max_ms_ - neutral_min_ms_ + 1U));
        *command_out = "neutral";
        return true;
    }

    static const char* expressions[] = {
        "happy", "sad", "angry", "love", "surprised", "confused",
        "sleepy", "curious", "nervous", "knocked",
        "glee", "worried", "focused", "annoyed", "skeptic",
        "frustrated", "unimpressed", "suspicious", "squint",
        "furious", "scared", "awe"
    };
    const int count = static_cast<int>(sizeof(expressions) / sizeof(expressions[0]));
    int idx = std::rand() % count;
    if (idx == last_shuffle_index_) {
        idx = (idx + 1) % count;
    }
    last_shuffle_index_ = idx;
    expression_phase_ = true;
    next_change_ms_ = now_ms + expr_min_ms_ + (std::rand() % (expr_max_ms_ - expr_min_ms_ + 1U));
    *command_out = expressions[idx];
    return true;
}

}  // namespace leor
