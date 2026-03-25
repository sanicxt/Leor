#pragma once

#include "esp_err.h"
#include "nvs.h"

#include <cstdint>
#include <string>

namespace leor {

class Preferences {
  public:
    Preferences() = default;
    ~Preferences();

    esp_err_t begin(const char* ns);
    void end();

    bool getBool(const char* key, bool fallback) const;
    int32_t getInt(const char* key, int32_t fallback) const;
    uint32_t getUInt(const char* key, uint32_t fallback) const;
    uint64_t getULong64(const char* key, uint64_t fallback) const;
    float getFloat(const char* key, float fallback) const;
    std::string getString(const char* key, const char* fallback) const;

    void putBool(const char* key, bool value);
    void putInt(const char* key, int32_t value);
    void putUInt(const char* key, uint32_t value);
    void putULong64(const char* key, uint64_t value);
    void putFloat(const char* key, float value);
    void putString(const char* key, const std::string& value);

  private:
    nvs_handle_t handle_ = 0;
    bool open_ = false;
};

}  // namespace leor
