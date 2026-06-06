cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD "defcon32_badge" CACHE STRING "Badge board name for SDK-compatible build metadata")
project(dc32_badge_firmware C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(APP DC32-cfw)
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(TINYUSB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyusb")
set(MINIZ_SRC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/pngle/src/miniz.c")
set(SIMAVR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/simavr")
set(ARDENS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ardens")
set(LINKER_SCRIPT "${SRC_DIR}/linker_rp2350_defcon.lkr")
set(DCAPP_LINKER_SCRIPT "${SRC_DIR}/linker_dcapp.lkr")
set(DCAPP_CACHE_START 0x10080000)
set(DCAPP_RAM_START 0x2005F000)
set(DCAPP_RAM_SIZE 0x00014000)

set(GB_RUNTIME_SOURCES
    ${SRC_DIR}/gb.c
    ${SRC_DIR}/gbC.c
    ${SRC_DIR}/mappersC.c
    ${SRC_DIR}/mbc.c
    ${SRC_DIR}/videoC.c
)

set(NES_RUNTIME_SOURCES
    ${SRC_DIR}/nes/InfoNES.cpp
    ${SRC_DIR}/nes/InfoNES_Mapper.cpp
    ${SRC_DIR}/nes/InfoNES_pAPU.cpp
    ${SRC_DIR}/nes/K6502.cpp
    ${SRC_DIR}/nes/nes_port.cpp
)

option(ARDUBOY_HYBRID_EXPERIMENTAL "Build the experimental ProjectABE-inspired Arduboy runtime instead of the default Ardens runtime" OFF)
option(ARDUBOY_SIMAVR_EXPERIMENTAL "Build the experimental simavr-derived Arduboy runtime instead of the default Ardens runtime" OFF)

set(ARDUBOY_RUNTIME_SOURCES)
set(ARDUBOY_SIMAVR_SOURCES)
set(ARDUBOY_ARDENS_SOURCES)
if(ARDUBOY_HYBRID_EXPERIMENTAL)
    list(APPEND ARDUBOY_RUNTIME_SOURCES
        ${SRC_DIR}/arduboy/arduboy_port.cpp
    )
elseif(ARDUBOY_SIMAVR_EXPERIMENTAL)
    list(APPEND ARDUBOY_RUNTIME_SOURCES
        ${SRC_DIR}/arduboy/arduboy_simavr_port.cpp
    )
    list(APPEND ARDUBOY_SIMAVR_SOURCES
        ${SIMAVR_DIR}/sim_fake_gdb.c
        ${SIMAVR_DIR}/examples/parts/ssd1306_virt.c
        ${SIMAVR_DIR}/simavr/sim/avr_acomp.c
        ${SIMAVR_DIR}/simavr/sim/avr_adc.c
        ${SIMAVR_DIR}/simavr/sim/avr_bitbang.c
        ${SIMAVR_DIR}/simavr/sim/avr_eeprom.c
        ${SIMAVR_DIR}/simavr/sim/avr_extint.c
        ${SIMAVR_DIR}/simavr/sim/avr_flash.c
        ${SIMAVR_DIR}/simavr/sim/avr_ioport.c
        ${SIMAVR_DIR}/simavr/sim/avr_lin.c
        ${SIMAVR_DIR}/simavr/sim/avr_spi.c
        ${SIMAVR_DIR}/simavr/sim/avr_timer.c
        ${SIMAVR_DIR}/simavr/sim/avr_twi.c
        ${SIMAVR_DIR}/simavr/sim/avr_uart.c
        ${SIMAVR_DIR}/simavr/sim/avr_usb.c
        ${SIMAVR_DIR}/simavr/sim/avr_watchdog.c
        ${SIMAVR_DIR}/simavr/sim/sim_avr.c
        ${SIMAVR_DIR}/simavr/sim/sim_cmds.c
        ${SIMAVR_DIR}/simavr/sim/sim_core.c
        ${SIMAVR_DIR}/simavr/sim/sim_cycle_timers.c
        ${SIMAVR_DIR}/simavr/sim/sim_interrupts.c
        ${SIMAVR_DIR}/simavr/sim/sim_io.c
        ${SIMAVR_DIR}/simavr/sim/sim_irq.c
        ${SIMAVR_DIR}/simavr/sim/sim_utils.c
        ${SIMAVR_DIR}/simavr/sim/sim_vcd_file.c
        ${SIMAVR_DIR}/simavr/cores/sim_mega32u4.c
    )
else()
    list(APPEND ARDUBOY_RUNTIME_SOURCES
        ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    )
    list(APPEND ARDUBOY_ARDENS_SOURCES
        ${ARDENS_DIR}/absim_embedded.cpp
        ${ARDENS_DIR}/absim_decode.cpp
        ${ARDENS_DIR}/absim_execute.cpp
        ${ARDENS_DIR}/absim_merge_instrs.cpp
        ${ARDENS_DIR}/absim_reset.cpp
    )
endif()

add_executable(${APP}
    ${SRC_DIR}/audioPwm.c
    ${SRC_DIR}/arduboy/arduboy_format.cpp
    ${SRC_DIR}/badgePower.c
    ${SRC_DIR}/badgeLeds.c
    ${SRC_DIR}/bootGuard.c
    ${SRC_DIR}/crt_rp2350.S
    ${SRC_DIR}/dcApp.c
    ${SRC_DIR}/dispDefcon.c
    ${SRC_DIR}/fatfs.c
    ${SRC_DIR}/fonts.c
    ${SRC_DIR}/gb_mbc_analyze.c
    ${SRC_DIR}/main_rp2350_defcon.c
    ${SRC_DIR}/nes/nes_analyze.c
    ${SRC_DIR}/pioI2C.c
    ${SRC_DIR}/pioIrdaSIR.c
    ${SRC_DIR}/pioWS2812.c
    ${SRC_DIR}/printf.c
    ${SRC_DIR}/qspi2350.c
    ${SRC_DIR}/sd.c
    ${SRC_DIR}/sdHwRP2350.c
    ${SRC_DIR}/settings.c
    ${SRC_DIR}/sleepDefcon.c
    ${SRC_DIR}/stringCompat.c
    ${SRC_DIR}/syscalls.c
    ${SRC_DIR}/timebase.c
    ${SRC_DIR}/toolWorkspace.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/usbDevice.c
    ${SRC_DIR}/usbHid.c
    ${SRC_DIR}/usbMsc.c
    ${SRC_DIR}/utf.c
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/dcd_rp2040.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/rp2040_usb.c
    ${MINIZ_SRC}
)

target_include_directories(${APP} PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/tinyusb_compat
    ${SRC_DIR}/inc_RP2350
    ${SRC_DIR}/arduboy
    ${ARDENS_DIR}
    ${SRC_DIR}/nes
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/pngle/src
    ${SIMAVR_DIR}
    ${SIMAVR_DIR}/simavr/sim
    ${SIMAVR_DIR}/simavr/cores
    ${SIMAVR_DIR}/examples/parts
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
    ARDUBOY_SIM_SCRATCH_SIZE=0x18000
    DISP_LCD_DEFCON
    UNSCALED_IMG_ROTATED
    UPSCALER_ROTATES
    UI_ROTATED
    GB_HAVE_EXTRA_IO_REGS
    PNGLE_NO_GAMMA_CORRECTION
    TICKS_PER_SECOND=125000000U
    ZWT_ADDR=0x20081ff8
    PICO_BOARD="${PICO_BOARD}"
    $<$<BOOL:${ARDUBOY_HYBRID_EXPERIMENTAL}>:ARDUBOY_HYBRID_EXPERIMENTAL=1>
    $<$<BOOL:${ARDUBOY_SIMAVR_EXPERIMENTAL}>:ARDUBOY_SIMAVR_EXPERIMENTAL=1>
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
    ${SRC_DIR}/arduboy/arduboy_port.cpp
    ${SRC_DIR}/arduboy/arduboy_simavr_port.cpp
    ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    ${SRC_DIR}/arduboy/arduboy_format.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${ARDUBOY_ARDENS_SOURCES}
    PROPERTIES
        COMPILE_DEFINITIONS "ARDENS_EMBEDDED=1;NDEBUG=1"
        COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    PROPERTIES
        COMPILE_DEFINITIONS "ARDENS_EMBEDDED=1;NDEBUG=1"
)

set_source_files_properties(
    ${ARDUBOY_SIMAVR_SOURCES}
    PROPERTIES
        COMPILE_DEFINITIONS "ARDUBOY_SIMAVR_NO_NAMES=1;ARDUBOY_SIMAVR_INSTANT_SPI=1;ARDUBOY_SIMAVR_DIRECT_SPI_SSD1306=1;ARDUBOY_SIMAVR_FAST_SPI_READY=1;ARDUBOY_SIMAVR_TRUST_IRQ_RAISE=1;ARDUBOY_SIMAVR_BATCH_INTERRUPTS_OFF=1;ARDUBOY_SIMAVR_NO_AUDIO_TIMERS=1;malloc=arduboySimMalloc;calloc=arduboySimCalloc;realloc=arduboySimRealloc;free=arduboySimFree;strdup=arduboySimStrdup;printf=arduboySimPrintf;fprintf=arduboySimFprintf;abort=arduboySimAbort;exit=arduboySimExit;usleep=arduboySimUsleep;getpid=arduboySimGetPid;random=arduboySimRandom;clock_gettime=arduboySimClockGettime;gettimeofday=arduboySimGettimeofday;fopen=arduboySimFopen;fclose=arduboySimFclose;fflush=arduboySimFflush"
        COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${MINIZ_SRC}
    PROPERTIES
        COMPILE_OPTIONS "-Wno-error"
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
    -Wl,--undefined=accessFail
    -Wl,--undefined=arduboyPortInGameMenu
    -Wl,--undefined=audioPwmSetVolume
    -Wl,--undefined=dispSetFramerate
    -Wl,--undefined=dispPrvWaitForScanoutStart
    -Wl,--undefined=gbDrawLine
    -Wl,--undefined=gbExtAccelRead
    -Wl,--undefined=gbExtGetKeys
    -Wl,--undefined=gbExtRtcGet
    -Wl,--undefined=gbExtRtcReset
    -Wl,--undefined=gbExtRtcSet
    -Wl,--undefined=gbExtraIoRegRead
    -Wl,--undefined=gbExtraIoRegWrite
    -Wl,--undefined=nesPortInGameMenu
    -Wl,--undefined=usbHidDropNow
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
        -D RAM_BUDGET=212992
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_size.cmake
    VERBATIM
)

set(DCAPP_HOST_SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/dcapp_host_symbols.ld")
add_custom_command(
    OUTPUT ${DCAPP_HOST_SYMBOLS}
    DEPENDS ${APP}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_host_symbols.py
        --nm ${CMAKE_NM}
        --elf $<TARGET_FILE:${APP}>
        --output ${DCAPP_HOST_SYMBOLS}
    VERBATIM
)
add_custom_target(dcapp_host_symbols DEPENDS ${DCAPP_HOST_SYMBOLS})

get_target_property(DC32_FIRMWARE_INCLUDE_DIRS ${APP} INCLUDE_DIRECTORIES)
get_target_property(DC32_FIRMWARE_COMPILE_DEFINITIONS ${APP} COMPILE_DEFINITIONS)
get_target_property(DC32_FIRMWARE_COMPILE_OPTIONS ${APP} COMPILE_OPTIONS)

function(add_dcapp TARGET_NAME OUTPUT_NAME RUNTIME_ID)
    set(RAW_FILE "${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.raw")
    set(DCAPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.DC32")

    add_executable(${TARGET_NAME} ${ARGN} ${SRC_DIR}/syscalls.c)
    add_dependencies(${TARGET_NAME} dcapp_host_symbols)
    target_include_directories(${TARGET_NAME} PRIVATE ${DC32_FIRMWARE_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_NAME} PRIVATE
        ${DC32_FIRMWARE_COMPILE_DEFINITIONS}
        DCAPP_BUILD=1
        DCAPP_TOOL_BUILD=1
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${DC32_FIRMWARE_COMPILE_OPTIONS})
    target_link_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
        -specs=nosys.specs
        -nostartfiles
        -Wl,--gc-sections
        -Wl,--undefined=dcAppAbort
        -Wl,--undefined=dcAppRefreshDisplayOptions
        -Wl,-T${DCAPP_LINKER_SCRIPT}
        -Wl,-T${DCAPP_HOST_SYMBOLS}
        -mthumb
        -mcpu=cortex-m33
        -mfloat-abi=soft
    )
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS ${DCAPP_HOST_SYMBOLS})
    target_link_libraries(${TARGET_NAME} PRIVATE m)

    add_custom_command(
        OUTPUT ${DCAPP_FILE}
        DEPENDS ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.dcapp
        COMMAND ${CMAKE_OBJCOPY} -O binary
            -j .dcapp_header -j .text -j .data
            $<TARGET_FILE:${TARGET_NAME}> ${RAW_FILE}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_dcapp.py
            --nm ${CMAKE_NM}
            --elf $<TARGET_FILE:${TARGET_NAME}>
            --raw ${RAW_FILE}
            --output ${DCAPP_FILE}
            --runtime-id ${RUNTIME_ID}
            --load-addr ${DCAPP_CACHE_START}
            --app-ram-start ${DCAPP_RAM_START}
            --app-ram-size ${DCAPP_RAM_SIZE}
        COMMAND ${CMAKE_SIZE} -A $<TARGET_FILE:${TARGET_NAME}>
        VERBATIM
    )
    add_custom_target(${TARGET_NAME}_dcapp ALL DEPENDS ${DCAPP_FILE})
    set_property(GLOBAL APPEND PROPERTY DCAPP_OUTPUTS ${DCAPP_FILE})
endfunction()

add_dcapp(dcapp_gb gb 1
    ${SRC_DIR}/apps/gb_app.c
    ${GB_RUNTIME_SOURCES}
)

add_dcapp(dcapp_nes nes 2
    ${SRC_DIR}/apps/nes_app.c
    ${NES_RUNTIME_SOURCES}
)

add_dcapp(dcapp_arduboy arduboy 3
    ${SRC_DIR}/apps/arduboy_app.c
    ${ARDUBOY_RUNTIME_SOURCES}
    ${ARDUBOY_SIMAVR_SOURCES}
    ${ARDUBOY_ARDENS_SOURCES}
)

add_dcapp(dcapp_ir ir 100
    ${SRC_DIR}/apps/ir_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/irRemote.c
)

add_dcapp(dcapp_image image 101
    ${SRC_DIR}/apps/image_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/imageViewer.c
    ${SRC_DIR}/raster.c
)

add_dcapp(dcapp_music music 102
    ${SRC_DIR}/apps/music_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/rtttlPlayer.c
)

add_dcapp(dcapp_badusb badusb 103
    ${SRC_DIR}/apps/badusb_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/badUsb.c
)
