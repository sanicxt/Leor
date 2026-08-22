idf_component_register(
    SRCS
        "src/application.cpp"
        "src/ble_service.cpp"
        "src/buzzer_service.cpp"
        "src/clock_service.cpp"
        "src/command_router.cpp"
        "src/display_backend.cpp"
        "src/gesture_service.cpp"
        "src/menu_service.cpp"
        "src/mochi_eyes_engine.cpp"
        "src/musical_score_encoder.c"
        "src/notification_overlay.cpp"
        "src/mpu6050_ahrs_ng.cpp"
        "src/ota_service.cpp"
        "src/power_service.cpp"
        "src/preferences.cpp"
        "src/shuffle_service.cpp"
        "src/wifi_time_sync_service.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        bt
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_rmt
        esp_hw_support
        esp_timer
        app_update
        log
        nvs_flash
        nixy4__u8g2
        freertos
        esp_wifi
        esp_netif
        lwip
        esp_coex
        esp_pm
        esp_event
)

# ── Auto-generate icon C headers from SVG assets ──────────────────────────────
set(ICON_OUT_DIR "${CMAKE_CURRENT_LIST_DIR}/include/leor/icons")
file(GLOB ICON_SVG_SRGS "${CMAKE_SOURCE_DIR}/assets/icons/icons8-*.svg")
# Prefer the repo venv (has PIL/cairosvg), fall back to system python3 (CI).
set(ICON_PYTHON "${CMAKE_SOURCE_DIR}/.venv/bin/python3")
if(NOT EXISTS "${ICON_PYTHON}")
    find_program(ICON_PYTHON_FOUND NAMES python3)
    if(ICON_PYTHON_FOUND)
        set(ICON_PYTHON "${ICON_PYTHON_FOUND}")
    endif()
endif()
add_custom_command(
    OUTPUT  ${ICON_OUT_DIR}/icons.h
    DEPENDS ${ICON_SVG_SRGS}
    COMMAND ${ICON_PYTHON}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/convert_icons.py
            --input ${CMAKE_SOURCE_DIR}/assets/icons
            --output ${ICON_OUT_DIR}
    COMMENT "Generating icon C headers from SVGs..."
)
add_custom_target(icons_generate DEPENDS ${ICON_OUT_DIR}/icons.h)
add_dependencies(${COMPONENT_LIB} icons_generate)
# ──────────────────────────────────────────────────────────────────────────────

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${IDF_PATH}/components/bt/host/nimble/nimble/nimble/host/include"
    "${IDF_PATH}/components/bt/host/nimble/nimble/nimble/include"
    "${IDF_PATH}/components/bt/host/nimble/nimble/porting/nimble/include"
    "${IDF_PATH}/components/bt/host/nimble/port/include"
)
