cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

list(APPEND EXTRA_COMPONENT_DIRS jmri_interface wifi_config withrottle_interface)

project(CommandStation-EX-ESP32)
