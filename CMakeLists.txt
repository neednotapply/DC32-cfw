cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD "defcon32_badge" CACHE STRING "Badge board name for SDK-compatible build metadata")
project(dc32_badge_firmware C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(APP DC32-cfw)
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(TINYUSB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyusb")
set(LINKER_SCRIPT "${SRC_DIR}/linker_rp2350_defcon.lkr")

add_executable(${APP}
    ${SRC_DIR}/audioPwm.c
    ${SRC_DIR}/badgeLeds.c
    ${SRC_DIR}/badUsb.c
    ${SRC_DIR}/bootGuard.c
    ${SRC_DIR}/crt_rp2350.S
    ${SRC_DIR}/dispDefcon.c
    ${SRC_DIR}/fatfs.c
    ${SRC_DIR}/fonts.c
    ${SRC_DIR}/gb.c
    ${SRC_DIR}/gbC.c
    ${SRC_DIR}/irRemote.c
    ${SRC_DIR}/imageViewer.c
    ${SRC_DIR}/main_rp2350_defcon.c
    ${SRC_DIR}/mappersC.c
    ${SRC_DIR}/mbc.c
    ${SRC_DIR}/nes/InfoNES.cpp
    ${SRC_DIR}/nes/InfoNES_Mapper.cpp
    ${SRC_DIR}/nes/InfoNES_pAPU.cpp
    ${SRC_DIR}/nes/K6502.cpp
    ${SRC_DIR}/nes/nes_port.cpp
    ${SRC_DIR}/pioI2C.c
    ${SRC_DIR}/pioIrdaSIR.c
    ${SRC_DIR}/pioWS2812.c
    ${SRC_DIR}/printf.c
    ${SRC_DIR}/qspi2350.c
    ${SRC_DIR}/raster.c
    ${SRC_DIR}/rtttlPlayer.c
    ${SRC_DIR}/sd.c
    ${SRC_DIR}/sdHwRP2350.c
    ${SRC_DIR}/settings.c
    ${SRC_DIR}/sleepDefcon.c
    ${SRC_DIR}/stringCompat.c
    ${SRC_DIR}/timebase.c
    ${SRC_DIR}/toolWorkspace.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/usbDevice.c
    ${SRC_DIR}/usbHid.c
    ${SRC_DIR}/usbMsc.c
    ${SRC_DIR}/utf.c
    ${SRC_DIR}/videoC.c
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/dcd_rp2040.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/rp2040_usb.c
)

target_include_directories(${APP} PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/tinyusb_compat
    ${SRC_DIR}/inc_RP2350
    ${SRC_DIR}/nes
    ${TINYUSB_DIR}/src
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040
)

target_compile_definitions(${APP} PRIVATE
    EMBEDDED
    NO_SUPPORT_DEBUG_PRINTF
    QUIET_MAPPERS
    FATFS_USE_LFN_SUPPORT=1
    USE_WIN_1252_CODE_PAGE
    QSPI_RAM_SIZE_MAX=65536
    DISP_LCD_DEFCON
    UNSCALED_IMG_ROTATED
    UPSCALER_ROTATES
    UI_ROTATED
    GB_HAVE_EXTRA_IO_REGS
    PNGLE_NO_GAMMA_CORRECTION
    TICKS_PER_SECOND=125000000U
    ZWT_ADDR=0x20081ff8
    PICO_BOARD="${PICO_BOARD}"
)

target_compile_options(${APP} PRIVATE
    -Ofast
    -Wall
    -Wextra
    -Werror
    -mthumb
    -fsingle-precision-constant
    -ffast-math
    -mcpu=cortex-m33
    -mfloat-abi=soft
    -ffixed-r11
    -fstack-reuse=all
    -ffunction-sections
    -fdata-sections
    -fomit-frame-pointer
    -fno-builtin-strcmp
    -Wno-array-bounds
    -Wno-clobbered
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-unused-variable
    -Wno-maybe-uninitialized
    -Wno-unused-label
    -Wno-missing-field-initializers
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-narrowing>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-old-style-cast>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-compare>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>
)

set_source_files_properties(
    ${SRC_DIR}/nes/InfoNES.cpp
    ${SRC_DIR}/nes/InfoNES_Mapper.cpp
    ${SRC_DIR}/nes/InfoNES_pAPU.cpp
    ${SRC_DIR}/nes/K6502.cpp
    ${SRC_DIR}/nes/nes_port.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/badUsb.c
    ${SRC_DIR}/settings.c
    ${SRC_DIR}/badgeLeds.c
    ${SRC_DIR}/imageViewer.c
    ${SRC_DIR}/raster.c
    PROPERTIES COMPILE_OPTIONS "-Os"
)

set_source_files_properties(
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/dcd_rp2040.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/rp2040_usb.c
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
)

target_link_options(${APP} PRIVATE
    -Wall
    -Wextra
    -specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-T${LINKER_SCRIPT}
    -mthumb
    -mcpu=cortex-m33
    -mfloat-abi=soft
)
target_link_libraries(${APP} PRIVATE m)

add_custom_command(TARGET ${APP} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${APP}> ${CMAKE_CURRENT_BINARY_DIR}/${APP}.bin -j.text -j.rodata -j.data -j.vectors
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bin_to_uf2.py
        ${CMAKE_CURRENT_BINARY_DIR}/${APP}.bin
        ${CMAKE_CURRENT_BINARY_DIR}/${APP}.uf2
        --base 0x10000000
        --family rp2350-arm-s
        --abs-block
    COMMAND ${CMAKE_SIZE} -A $<TARGET_FILE:${APP}>
    COMMAND ${CMAKE_COMMAND}
        -D SIZE_TOOL=${CMAKE_SIZE}
        -D ELF_FILE=$<TARGET_FILE:${APP}>
        -D RAM_BUDGET=307200
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_size.cmake
    VERBATIM
)
