cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -Wno-format -Wno-unused-function -Wno-maybe-uninitialized)

# Initialize the SDK
pico_sdk_init()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

set(project_name PICO_GB_Bootloader)
project(${project_name})

set(src src/main.c)
set(headers "")

file(GLOB roms "roms/*.gb")

# Generate rom files
configure_file(scripts/rom_to_header.py ${CMAKE_CURRENT_BINARY_DIR}/rom_to_header COPYONLY)

foreach(gb_rom ${roms})
  get_filename_component(rom_name ${gb_rom} NAME_WLE)
  set(rom_output ${CMAKE_CURRENT_BINARY_DIR}/include/roms/${rom_name}.h)
  add_custom_target(
    always_run_target ALL
    DEPENDS ${rom_output}
  )

  add_custom_command(
    OUTPUT ${rom_output}
    DEPENDS ${gb_rom}
    COMMAND ./rom_to_header
    ARGS ${gb_rom} ${rom_output}
  )

  set(headers ${headers} ${rom_output})
endforeach()

add_executable(${project_name} ${src} ${headers})
target_include_directories(${project_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

# Pull in our pico_stdlib which aggregates commonly used features
target_link_libraries(${project_name} pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(${project_name})

pico_set_binary_type(${project_name} copy_to_ram)