#pragma once

#include <cstdint>

namespace leor {

class ShuffleService {
  public:
    void restore(bool enabled, uint32_t expr_min_ms, uint32_t expr_max_ms, uint32_t neutral_min_ms, uint32_t neutral_max_ms);
    void reset();
    bool enabled() const { return enabled_; }
    void set_enabled(bool enabled);
    void set_expr_range(uint32_t min_ms, uint32_t max_ms);
    void set_neutral_range(uint32_t min_ms, uint32_t max_ms);
    bool should_emit(uint32_t now_ms, bool reacting, bool training, const char** command_out);
    uint32_t expr_min_ms() const { return expr_min_ms_; }
    uint32_t expr_max_ms() const { return expr_max_ms_; }
    uint32_t neutral_min_ms() const { return neutral_min_ms_; }
    uint32_t neutral_max_ms() const { return neutral_max_ms_; }

  private:
    bool enabled_ = true;
    bool needs_init_ = true;
    bool expression_phase_ = false;
    uint32_t expr_min_ms_ = 2000;
    uint32_t expr_max_ms_ = 5000;
    uint32_t neutral_min_ms_ = 2000;
    uint32_t neutral_max_ms_ = 5000;
    uint32_t next_change_ms_ = 0;
    int last_shuffle_index_ = -1;
};

}  // namespace leor
