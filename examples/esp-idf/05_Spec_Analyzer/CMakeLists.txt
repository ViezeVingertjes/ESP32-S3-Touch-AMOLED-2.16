# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    ./components/bsp_extra
    )

add_compile_options(-Wno-format)
project(lvgl_demo_v9)

# shortcut: esp-audio-player 2.0.0 leaves http_stream_set_eof() unused when
# CONFIG_AUDIO_PLAYER_ENABLE_HTTP_STREAM=n; drop this once upstream guards it.
idf_component_get_property(audio_player_lib chmorgan__esp-audio-player COMPONENT_LIB)
target_compile_options(${audio_player_lib} PRIVATE -Wno-unused-function)
