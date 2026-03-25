cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(leor_idf)

# IDF v6.0 split the 'driver' umbrella into separate components.
# The nixy4/u8g2 managed component includes driver/gpio.h in its SPI port
# file, which now lives in esp_driver_gpio. Inject it as a public dep
# so u8g2 can find the header without modifying the managed component.
idf_component_get_property(u8g2_lib nixy4__u8g2 COMPONENT_LIB)
target_link_libraries(${u8g2_lib} PUBLIC idf::esp_driver_gpio)
