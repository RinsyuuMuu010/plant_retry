cmake_minimum_required(VERSION 3.15)
project(plant_retry VERSION 3.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 单配置生成器（Ninja / MinGW Makefiles）没给构建类型时默认 Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(plant_retry
        main.c
        pgsm.c
        ui.c
        save.c
        pgsm.h
        ui.h
)

if(MSVC)
    target_compile_options(plant_retry PRIVATE /W4)
else()
    target_compile_options(plant_retry PRIVATE -Wall -Wextra -Wpedantic)
endif()
