#include "leor/preferences.hpp"

#include <cstring>

namespace leor {

Preferences::~Preferences() {
    end();
}

esp_err_t Preferences::begin(const char* ns) {
    end();
    const esp_err_t err = nvs_open(ns, NVS_READWRITE, &handle_);
    open_ = (err == ESP_OK);
    return err;
}

void Preferences::end() {
    if (open_) {
        nvs_close(handle_);
        handle_ = 0;
        open_ = false;
    }
}

bool Preferences::getBool(const char* key, bool fallback) const {
    uint8_t value = fallback ? 1 : 0;
    if (!open_ || nvs_get_u8(handle_, key, &value) != ESP_OK) {
        return fallback;
    }
    return value != 0;
}

int32_t Preferences::getInt(const char* key, int32_t fallback) const {
    int32_t value = fallback;
    if (!open_ || nvs_get_i32(handle_, key, &value) != ESP_OK) {
        return fallback;
    }
    return value;
}

uint32_t Preferences::getUInt(const char* key, uint32_t fallback) const {
    uint32_t value = fallback;
    if (!open_ || nvs_get_u32(handle_, key, &value) != ESP_OK) {
        return fallback;
    }
    return value;
}

uint64_t Preferences::getULong64(const char* key, uint64_t fallback) const {
    uint64_t value = fallback;
    if (!open_ || nvs_get_u64(handle_, key, &value) != ESP_OK) {
        return fallback;
    }
    return value;
}

float Preferences::getFloat(const char* key, float fallback) const {
    uint32_t raw = 0;
    if (!open_ || nvs_get_u32(handle_, key, &raw) != ESP_OK) {
        return fallback;
    }
    float value = 0.0f;
    static_assert(sizeof(value) == sizeof(raw));
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

std::string Preferences::getString(const char* key, const char* fallback) const {
    if (!open_) {
        return fallback;
    }
    size_t length = 0;
    if (nvs_get_str(handle_, key, nullptr, &length) != ESP_OK || length == 0) {
        return fallback;
    }
    std::string value(length, '\0');
    if (nvs_get_str(handle_, key, value.data(), &length) != ESP_OK) {
        return fallback;
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();
    }
    return value;
}

void Preferences::putBool(const char* key, bool value) {
    if (open_) {
        nvs_set_u8(handle_, key, value ? 1 : 0);
        nvs_commit(handle_);
    }
}

void Preferences::putInt(const char* key, int32_t value) {
    if (open_) {
        nvs_set_i32(handle_, key, value);
        nvs_commit(handle_);
    }
}

void Preferences::putUInt(const char* key, uint32_t value) {
    if (open_) {
        nvs_set_u32(handle_, key, value);
        nvs_commit(handle_);
    }
}

void Preferences::putULong64(const char* key, uint64_t value) {
    if (open_) {
        nvs_set_u64(handle_, key, value);
        nvs_commit(handle_);
    }
}

void Preferences::putFloat(const char* key, float value) {
    if (open_) {
        uint32_t raw = 0;
        std::memcpy(&raw, &value, sizeof(raw));
        nvs_set_u32(handle_, key, raw);
        nvs_commit(handle_);
    }
}

void Preferences::putString(const char* key, const std::string& value) {
    if (open_) {
        nvs_set_str(handle_, key, value.c_str());
        nvs_commit(handle_);
    }
}

}  // namespace leor
