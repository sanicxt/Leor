idf_component_register(
    SRCS
        "src/application.cpp"
        "src/ble_service.cpp"
        "src/clock_service.cpp"
        "src/command_router.cpp"
        "src/display_backend.cpp"
        "src/gesture_service.cpp"
        "src/menu_service.cpp"
        "src/mochi_eyes_engine.cpp"
        "src/mpu6050_ahrs_ng.cpp"
        "src/ota_service.cpp"
        "src/power_service.cpp"
        "src/preferences.cpp"
        "src/shuffle_service.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        bt
        driver
        esp_timer
        app_update
        log
        nvs_flash
        nixy4__u8g2
        freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_include_directories(${COMPONENT_LIB} PRIVATE
    "${IDF_PATH}/components/bt/host/nimble/nimble/nimble/host/include"
    "${IDF_PATH}/components/bt/host/nimble/nimble/nimble/include"
    "${IDF_PATH}/components/bt/host/nimble/nimble/porting/nimble/include"
    "${IDF_PATH}/components/bt/host/nimble/port/include"
)
