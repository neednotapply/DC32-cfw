cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD "defcon32_badge" CACHE STRING "Badge board name for SDK-compatible build metadata")
project(dc32_badge_firmware C CXX ASM)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
option(DC32_NES_PERF_PROFILE "Enable NES on-device performance logging" OFF)
option(DC32_ARDUBOY_HLE "Enable legacy routine-specific Arduboy graphics HLE" OFF)

set(NES_PERF_ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/nes-perf")
set(NES_CPU_PERF_ROM "${NES_PERF_ROM_DIR}/nes_cpu_perf.nes")
set(NES_PPU_PERF_ROM "${NES_PERF_ROM_DIR}/nes_ppu_perf.nes")
add_custom_command(
    OUTPUT ${NES_CPU_PERF_ROM} ${NES_PPU_PERF_ROM}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_nes_perf_rom.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${NES_PERF_ROM_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_nes_perf_rom.py cpu ${NES_CPU_PERF_ROM}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_nes_perf_rom.py ppu ${NES_PPU_PERF_ROM}
    VERBATIM
)
add_custom_target(nes_perf_roms DEPENDS ${NES_CPU_PERF_ROM} ${NES_PPU_PERF_ROM})

set(ARDUBOY_PERF_ROM_DIR "${CMAKE_CURRENT_BINARY_DIR}/arduboy-perf")
set(ARDUBOY_CPU_PERF_ROM "${ARDUBOY_PERF_ROM_DIR}/arduboy_cpu_perf.hex")
set(ARDUBOY_SPI_PERF_ROM "${ARDUBOY_PERF_ROM_DIR}/arduboy_spi_perf.hex")
set(ARDUBOY_TIMER_PERF_ROM "${ARDUBOY_PERF_ROM_DIR}/arduboy_timer_perf.hex")
set(ARDUBOY_GRAPHICS_PERF_ROM "${ARDUBOY_PERF_ROM_DIR}/arduboy_graphics_perf.hex")
set(ARDUBOY_FX_PERF_ROM "${ARDUBOY_PERF_ROM_DIR}/arduboy_fx_perf.arduboy")
add_custom_command(
    OUTPUT ${ARDUBOY_CPU_PERF_ROM} ${ARDUBOY_SPI_PERF_ROM} ${ARDUBOY_TIMER_PERF_ROM} ${ARDUBOY_GRAPHICS_PERF_ROM} ${ARDUBOY_FX_PERF_ROM}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ARDUBOY_PERF_ROM_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py cpu ${ARDUBOY_CPU_PERF_ROM}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py spi ${ARDUBOY_SPI_PERF_ROM}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py timer ${ARDUBOY_TIMER_PERF_ROM}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py graphics ${ARDUBOY_GRAPHICS_PERF_ROM}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arduboy_perf_hex.py fx ${ARDUBOY_FX_PERF_ROM}
    VERBATIM
)
add_custom_target(arduboy_perf_roms DEPENDS
    ${ARDUBOY_CPU_PERF_ROM} ${ARDUBOY_SPI_PERF_ROM} ${ARDUBOY_TIMER_PERF_ROM} ${ARDUBOY_GRAPHICS_PERF_ROM} ${ARDUBOY_FX_PERF_ROM})

set(APP DC32-cfw)
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(TINYUSB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyusb")
set(MINIZ_SRC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/pngle/src/miniz.c")
set(PICOJPEG_SRC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/picojpeg/picojpeg.c")
set(GIFDEC_SRC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/gifdec/gifdec.c")
set(ARDENS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ardens")
set(LINKER_SCRIPT "${SRC_DIR}/linker_rp2350_defcon.lkr")
set(DCAPP_LINKER_SCRIPT "${SRC_DIR}/linker_dcapp.lkr")
set(DCAPP_EH_LINKER_SCRIPT "${SRC_DIR}/linker_dcapp_eh.lkr")
set(DCAPP_LARGE_LINKER_SCRIPT "${SRC_DIR}/linker_dcapp_large.lkr")
set(DCAPP_LARGE_EH_LINKER_SCRIPT "${SRC_DIR}/linker_dcapp_large_eh.lkr")
set(DCAPP_CACHE_START 0x10080000)
set(DCAPP_LARGE_LOAD_START 0x10100000)
set(DCAPP_RAM_START 0x2005F000)
set(DCAPP_RAM_SIZE 0x00014000)

file(STRINGS "${SRC_DIR}/dcApp.h" DCAPP_ABI_VERSION_DEFINE
    REGEX "^#define[ \t]+DCAPP_SDK_ABI[ \t]+[0-9]+u?")
string(REGEX MATCH "[0-9]+" DCAPP_ABI_VERSION "${DCAPP_ABI_VERSION_DEFINE}")
if(NOT DCAPP_ABI_VERSION)
    message(FATAL_ERROR "Could not determine DCAPP_ABI_VERSION from src/dcApp.h")
endif()

set(DCAPP_ABI_VENEERS "${CMAKE_CURRENT_BINARY_DIR}/dcapp_abi_veneers.S")
add_custom_command(
    OUTPUT ${DCAPP_ABI_VENEERS}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_abi_veneers.py
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_abi_veneers.py
        --output ${DCAPP_ABI_VENEERS}
    VERBATIM
)
add_custom_target(dcapp_abi_veneers DEPENDS ${DCAPP_ABI_VENEERS})

set(DCAPP_SDK_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/DC32-App-SDK-v${DCAPP_ABI_VERSION}.zip")
add_custom_command(
    OUTPUT ${DCAPP_SDK_ARCHIVE}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_dcapp_sdk.py
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_abi_veneers.py
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_dcapp.py
		${CMAKE_CURRENT_SOURCE_DIR}/sdk/DC32App.cmake
        ${SRC_DIR}/dcApp.h
		${SRC_DIR}/syscalls.c
        ${SRC_DIR}/linker_dcapp.lkr
        ${SRC_DIR}/linker_dcapp_large.lkr
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_dcapp_sdk.py
        --output ${DCAPP_SDK_ARCHIVE}
    VERBATIM
)
add_custom_target(dcapp_sdk DEPENDS ${DCAPP_SDK_ARCHIVE})

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
    ${SRC_DIR}/nes/K6502.cpp
    ${SRC_DIR}/nes/nes_port.cpp
)

set(ARDUBOY_RUNTIME_SOURCES
    ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    ${ARDENS_DIR}/absim_embedded.cpp
    ${ARDENS_DIR}/absim_decode.cpp
    ${ARDENS_DIR}/absim_execute.cpp
    ${ARDENS_DIR}/absim_merge_instrs.cpp
    ${ARDENS_DIR}/absim_reset.cpp
)

add_executable(${APP}
	${DCAPP_ABI_VENEERS}
    ${SRC_DIR}/audioPwm.c
    ${SRC_DIR}/arduboy/arduboy_format.cpp
    ${SRC_DIR}/badgePower.c
    ${SRC_DIR}/badgeLeds.c
    ${SRC_DIR}/bootGuard.c
    ${SRC_DIR}/crt_rp2350.S
    ${SRC_DIR}/dcApp.c
    ${SRC_DIR}/dcAppDraw.c
    ${SRC_DIR}/dispDefcon.c
    ${SRC_DIR}/fatfs.c
    ${SRC_DIR}/fonts.c
    ${SRC_DIR}/gb_mbc_analyze.c
    ${SRC_DIR}/imageViewer.c
    ${SRC_DIR}/main_rp2350_defcon.c
    ${SRC_DIR}/irRemoteRx.c
    ${SRC_DIR}/nes/nes_analyze.c
    ${SRC_DIR}/pioI2C.c
    ${SRC_DIR}/pioIrdaSIR.c
    ${SRC_DIR}/pioWS2812.c
    ${SRC_DIR}/printf.c
    ${SRC_DIR}/qspi2350.c
    ${SRC_DIR}/raster.c
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

    ${SRC_DIR}/usbCdc.c
    ${SRC_DIR}/usbHid.c
    ${SRC_DIR}/usbMsc.c
    ${SRC_DIR}/usbXinput.c
    ${SRC_DIR}/utf.c
    ${PICOJPEG_SRC}
    ${GIFDEC_SRC}
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/cdc/cdc_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
    ${TINYUSB_DIR}/src/common/tusb_fifo.c
    ${TINYUSB_DIR}/src/device/usbd.c
    ${TINYUSB_DIR}/src/device/usbd_control.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/dcd_rp2040.c
    ${TINYUSB_DIR}/src/portable/raspberrypi/rp2040/rp2040_usb.c
    ${MINIZ_SRC}
)

target_include_directories(${APP} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${SRC_DIR}
    ${SRC_DIR}/tinyusb_compat
    ${SRC_DIR}/inc_RP2350
    ${SRC_DIR}/arduboy
    ${ARDENS_DIR}
    ${SRC_DIR}/nes
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/pngle/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/picojpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/image_decoders/gifdec
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
)
add_dependencies(${APP} dcapp_abi_veneers)

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
    ${SRC_DIR}/nes/K6502.cpp
    ${SRC_DIR}/nes/nes_port.cpp
    ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    ${SRC_DIR}/arduboy/arduboy_format.cpp
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${ARDUBOY_RUNTIME_SOURCES}
    PROPERTIES
        COMPILE_DEFINITIONS "ARDENS_EMBEDDED=1;ARDENS_GAME_ONLY=1;NDEBUG=1"
        COMPILE_OPTIONS "-Wno-error"
)

set_source_files_properties(
    ${SRC_DIR}/arduboy/arduboy_ardens_port.cpp
    PROPERTIES
        COMPILE_DEFINITIONS "ARDENS_EMBEDDED=1;ARDENS_GAME_ONLY=1;NDEBUG=1"
)

set_source_files_properties(
    ${MINIZ_SRC}
    ${PICOJPEG_SRC}
    ${GIFDEC_SRC}
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
    ${SRC_DIR}/dcAppDraw.c
    ${SRC_DIR}/apps/demos.c
    ${SRC_DIR}/apps/tetris/tetris_core.c
    ${SRC_DIR}/apps/tetris/tetris_rules.c
    ${SRC_DIR}/apps/tetris/tetris_port.c
    ${SRC_DIR}/apps/trex/trex_port.c
    PROPERTIES COMPILE_OPTIONS "-Os"
)

set_source_files_properties(
    ${SRC_DIR}/apps/flappy/flappy_port.c
    PROPERTIES COMPILE_OPTIONS "-O2"
)

set_source_files_properties(
    ${TINYUSB_DIR}/src/tusb.c
    ${TINYUSB_DIR}/src/class/hid/hid_device.c
    ${TINYUSB_DIR}/src/class/msc/msc_device.c
    ${TINYUSB_DIR}/src/class/vendor/vendor_device.c
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
    -Wl,--undefined=audioPwmGetVolume
    -Wl,--undefined=audioPwmStop
    -Wl,--undefined=audioPwmTone
    -Wl,--undefined=audioPwmPcmStart
    -Wl,--undefined=audioPwmPcmCanWrite
    -Wl,--undefined=audioPwmPcmWriteU8
    -Wl,--undefined=audioPwmPcmDrain
    -Wl,--undefined=audioPwmPcmStop
    -Wl,--undefined=badgeLedsOverrideRgb
    -Wl,--undefined=dcAppDrawRgb565
    -Wl,--undefined=dcAppDrawInit
    -Wl,--undefined=dcAppDrawClear
    -Wl,--undefined=dcAppDrawFill
    -Wl,--undefined=dcAppDrawPixel
    -Wl,--undefined=dcAppDrawLine
    -Wl,--undefined=dcAppDrawIcon
    -Wl,--undefined=dcAppDrawIconCanvas
    -Wl,--undefined=dcAppDrawPresent
    -Wl,--undefined=dcAppDrawFrame
    -Wl,--undefined=dcAppDrawWaitRelease
    -Wl,--undefined=dcAppDrawLoading
    -Wl,--undefined=dcAppDrawLoadingCanvas
    -Wl,--undefined=dcAppCore1Start
    -Wl,--undefined=dcAppCore1Join
    -Wl,--undefined=dcAppCore1ForceStop
    -Wl,--undefined=dispSetFramerate
    -Wl,--undefined=dispPrvFrameCtrWait
    -Wl,--undefined=dispPrvWaitForScanoutStart
    -Wl,--undefined=fatfsFileDelete
    -Wl,--undefined=gbDrawLine
    -Wl,--undefined=gbExtAccelRead
    -Wl,--undefined=gbExtGetKeys
    -Wl,--undefined=gbExtRtcGet
    -Wl,--undefined=gbExtRtcReset
    -Wl,--undefined=gbExtRtcSet
    -Wl,--undefined=gbExtraIoRegRead
    -Wl,--undefined=gbExtraIoRegWrite
    -Wl,--undefined=nesPortInGameMenu
    -Wl,--undefined=uiPowerSetScreenSaverContentActive
    -Wl,--undefined=usbHidBeginReportSet
    -Wl,--undefined=usbHidConsumerReport
    -Wl,--undefined=usbHidDropNow
    -Wl,--undefined=usbHidGamepadReport
    -Wl,--undefined=usbHidGamepadReportTouch
    -Wl,--undefined=usbHidGamepadFeedback
    -Wl,--undefined=usbHidMouseReport
    -Wl,--undefined=usbCdcAvailable
    -Wl,--undefined=usbCdcBegin
    -Wl,--undefined=usbCdcConnected
    -Wl,--undefined=usbCdcDefaultInfo
    -Wl,--undefined=usbCdcEnd
    -Wl,--undefined=usbCdcFlush
    -Wl,--undefined=usbCdcLastError
    -Wl,--undefined=usbCdcRead
    -Wl,--undefined=usbCdcTask
    -Wl,--undefined=usbCdcWrite
    -Wl,--undefined=usbCdcWriteAvailable
    -Wl,--undefined=usbXinputBegin
    -Wl,--undefined=usbXinputEnd
    -Wl,--undefined=usbXinputFeedback
    -Wl,--undefined=usbXinputLastError
    -Wl,--undefined=usbXinputReady
    -Wl,--undefined=usbXinputReleaseAll
    -Wl,--undefined=usbXinputReport
    -Wl,--undefined=usbXinputSetReportsEnabled
    -Wl,--undefined=usbXinputStarted
    -Wl,--undefined=usbXinputTask
    -Wl,-T${LINKER_SCRIPT}
    -mthumb
    -mcpu=cortex-m33
    -mfloat-abi=soft
)
target_link_libraries(${APP} PRIVATE m)

add_custom_command(TARGET ${APP} POST_BUILD
    # SDK-v1 apps import through fixed veneers at 0x10070000.  Keep this
    # section in the flat image so the UF2 programs it on the badge.
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${APP}> ${CMAKE_CURRENT_BINARY_DIR}/${APP}.bin -j.text -j.rodata -j.data -j.vectors -j.dcapp_abi_veneer
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
    DEPENDS ${APP} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_host_symbols.py
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_dcapp_host_symbols.py
        --nm ${CMAKE_NM}
        --elf $<TARGET_FILE:${APP}>
        --output ${DCAPP_HOST_SYMBOLS}
		--veneer-prefix dcapp_abi_veneer_
    VERBATIM
)
add_custom_target(dcapp_host_symbols DEPENDS ${DCAPP_HOST_SYMBOLS})

get_target_property(DC32_FIRMWARE_INCLUDE_DIRS ${APP} INCLUDE_DIRECTORIES)
get_target_property(DC32_FIRMWARE_COMPILE_DEFINITIONS ${APP} COMPILE_DEFINITIONS)
get_target_property(DC32_FIRMWARE_COMPILE_OPTIONS ${APP} COMPILE_OPTIONS)

function(add_dcapp TARGET_NAME OUTPUT_NAME RUNTIME_ID)
    cmake_parse_arguments(DCAPP "LARGE_XIP;CXX_EXCEPTIONS" "CATEGORY" "" ${ARGN})
    if(NOT DCAPP_CATEGORY)
        set(DCAPP_CATEGORY ports)
    endif()
    if(DCAPP_LARGE_XIP)
        if(DCAPP_CXX_EXCEPTIONS)
            set(DCAPP_TARGET_LINKER_SCRIPT "${DCAPP_LARGE_EH_LINKER_SCRIPT}")
        else()
            set(DCAPP_TARGET_LINKER_SCRIPT "${DCAPP_LARGE_LINKER_SCRIPT}")
        endif()
        set(DCAPP_TARGET_LOAD_ADDR "${DCAPP_LARGE_LOAD_START}")
        set(DCAPP_TARGET_FLAGS 1)
    else()
        if(DCAPP_CXX_EXCEPTIONS)
            set(DCAPP_TARGET_LINKER_SCRIPT "${DCAPP_EH_LINKER_SCRIPT}")
        else()
            set(DCAPP_TARGET_LINKER_SCRIPT "${DCAPP_LINKER_SCRIPT}")
        endif()
        set(DCAPP_TARGET_LOAD_ADDR "${DCAPP_CACHE_START}")
        set(DCAPP_TARGET_FLAGS 0)
    endif()
    set(RAW_FILE "${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.raw")
    set(DCAPP_FILE "${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.DC32")

    add_executable(${TARGET_NAME} ${DCAPP_UNPARSED_ARGUMENTS} ${SRC_DIR}/syscalls.c)
    add_dependencies(${TARGET_NAME} dcapp_host_symbols)
    target_include_directories(${TARGET_NAME} PRIVATE ${DC32_FIRMWARE_INCLUDE_DIRS})
    target_compile_definitions(${TARGET_NAME} PRIVATE
        ${DC32_FIRMWARE_COMPILE_DEFINITIONS}
        DCAPP_BUILD=1
        DCAPP_TOOL_BUILD=1
        DCAPP_RUNTIME_ID=${RUNTIME_ID}
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${DC32_FIRMWARE_COMPILE_OPTIONS})
    if(DCAPP_CXX_EXCEPTIONS)
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
            $<$<COMPILE_LANGUAGE:CXX>:-funwind-tables>
        )
    endif()
    target_link_options(${TARGET_NAME} PRIVATE
        -Wall
        -Wextra
        -specs=nosys.specs
        -nostartfiles
        -Wl,--gc-sections
        -Wl,--undefined=dcAppAbort
        -Wl,--undefined=dcAppRefreshDisplayOptions
        -Wl,-T${DCAPP_TARGET_LINKER_SCRIPT}
        -Wl,-T${DCAPP_HOST_SYMBOLS}
        -mthumb
        -mcpu=cortex-m33
        -mfloat-abi=soft
    )
    if(DCAPP_CXX_EXCEPTIONS)
        target_link_options(${TARGET_NAME} PRIVATE -fexceptions)
    endif()
    set_property(TARGET ${TARGET_NAME} APPEND PROPERTY LINK_DEPENDS ${DCAPP_HOST_SYMBOLS})
    target_link_libraries(${TARGET_NAME} PRIVATE m)
    set(DCAPP_RAW_SECTIONS -j .dcapp_header -j .text)
    if(DCAPP_CXX_EXCEPTIONS)
        list(APPEND DCAPP_RAW_SECTIONS
            -j .preinit_array
            -j .init_array
            -j .fini_array
            -j .ARM.extab
            -j .ARM.exidx
        )
    endif()
    list(APPEND DCAPP_RAW_SECTIONS -j .data)

    add_custom_command(
        OUTPUT ${DCAPP_FILE}
        DEPENDS ${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
        COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/apps/${OUTPUT_NAME}.dcapp
        COMMAND ${CMAKE_OBJCOPY} -O binary
            ${DCAPP_RAW_SECTIONS}
            $<TARGET_FILE:${TARGET_NAME}> ${RAW_FILE}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/make_dcapp.py
            --nm ${CMAKE_NM}
            --elf $<TARGET_FILE:${TARGET_NAME}>
            --raw ${RAW_FILE}
            --output ${DCAPP_FILE}
            --abi-version ${DCAPP_ABI_VERSION}
            --runtime-id ${RUNTIME_ID}
            --load-addr ${DCAPP_TARGET_LOAD_ADDR}
            --flags ${DCAPP_TARGET_FLAGS}
            --app-ram-start ${DCAPP_RAM_START}
            --app-ram-size ${DCAPP_RAM_SIZE}
			--app-name ${OUTPUT_NAME}
			--category ${DCAPP_CATEGORY}
        COMMAND ${CMAKE_SIZE} -A $<TARGET_FILE:${TARGET_NAME}>
        VERBATIM
    )
    add_custom_target(${TARGET_NAME}_dcapp ALL DEPENDS ${DCAPP_FILE})
    set_property(GLOBAL APPEND PROPERTY DCAPP_OUTPUTS ${DCAPP_FILE})
endfunction()

add_dcapp(dcapp_gb gb 1
	CATEGORY games
    ${SRC_DIR}/apps/gb_app.c
    ${GB_RUNTIME_SOURCES}
)

add_dcapp(dcapp_nes nes 2
	CATEGORY games
    ${SRC_DIR}/apps/nes_app.c
    ${NES_RUNTIME_SOURCES}
)
target_compile_definitions(dcapp_nes PRIVATE NES_FASTCODE_ENABLED=1)
if(DC32_NES_PERF_PROFILE)
    target_compile_definitions(dcapp_nes PRIVATE NES_PERF_PROFILE=1)
endif()

add_dcapp(dcapp_arduboy arduboy 3
	CATEGORY games
    ${SRC_DIR}/apps/arduboy_app.c
    ${ARDUBOY_RUNTIME_SOURCES}
)
if(DC32_ARDUBOY_HLE)
    target_compile_definitions(dcapp_arduboy PRIVATE ARDUBOY_HLE_ENABLED=1)
endif()

add_dcapp(dcapp_ir ir 100
	CATEGORY infrared
    ${SRC_DIR}/apps/ir_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/irRemote.c
)

add_dcapp(dcapp_image image 101
	CATEGORY media
    ${SRC_DIR}/apps/image_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/imageViewer.c
    ${SRC_DIR}/raster.c
    ${PICOJPEG_SRC}
    ${GIFDEC_SRC}
)

add_dcapp(dcapp_music music 102
	CATEGORY media
    ${SRC_DIR}/apps/music_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/abcPlayer.c
    ${SRC_DIR}/midiPlayer.c
    ${SRC_DIR}/rtttlPlayer.c
)

add_dcapp(dcapp_badusb badusb 103
	CATEGORY usb
    ${SRC_DIR}/apps/badusb_app.c
    ${SRC_DIR}/ui.c
    ${SRC_DIR}/badUsb.c
)

add_dcapp(dcapp_autoclicker autoclicker 104
	CATEGORY usb
    ${SRC_DIR}/apps/autoclicker_app.c
    ${SRC_DIR}/ui.c
)

add_dcapp(dcapp_gamepad gamepad 105
	CATEGORY usb
    ${SRC_DIR}/apps/gamepad_app.c
    ${SRC_DIR}/ui.c
)

add_dcapp(dcapp_lasertag lasertag 106
	CATEGORY infrared
    ${SRC_DIR}/apps/lasertag/lasertag_app.c
    ${SRC_DIR}/apps/lasertag/lasertag.c
    ${SRC_DIR}/dcAppDraw.c
    ${SRC_DIR}/irRemote.c
)

add_dcapp(dcapp_raspyjack raspyjack 107
	CATEGORY usb
    ${SRC_DIR}/apps/raspyjack_app.c
    ${SRC_DIR}/apps/raspyjack_remote.c
    ${SRC_DIR}/raster.c
    ${PICOJPEG_SRC}
    ${GIFDEC_SRC}
)

add_dcapp(dcapp_pwnagotchi pwnagotchi 108
	CATEGORY usb
    ${SRC_DIR}/apps/pwnagotchi_app.c
    ${SRC_DIR}/apps/pwnagotchi_remote.c
    ${SRC_DIR}/raster.c
)

target_compile_definitions(dcapp_lasertag PRIVATE
    NDEBUG=1
)

set(DEMOS_APP_SOURCES
    ${SRC_DIR}/apps/demos_app.c
    ${SRC_DIR}/apps/demos.c
)

set(PONG_APP_SOURCES
    ${SRC_DIR}/apps/pong/pong_app.c
    ${SRC_DIR}/apps/pong/main/pong_main.c
    ${SRC_DIR}/apps/pong/core/pong_core.c
    ${SRC_DIR}/apps/pong/platform/pong_dc32.c
    ${SRC_DIR}/apps/pong/modes/pong_modes.c
    ${SRC_DIR}/apps/pong/modes/pong_mode_common.c
    ${SRC_DIR}/apps/pong/modes/mode_classic.c
    ${SRC_DIR}/apps/pong/modes/mode_doubles.c
    ${SRC_DIR}/apps/pong/modes/mode_quadrapong.c
    ${SRC_DIR}/apps/pong/modes/mode_super.c
    ${SRC_DIR}/apps/pong/modes/mode_rebound.c
    ${SRC_DIR}/apps/pong/modes/mode_breakout.c
    ${SRC_DIR}/apps/pong/modes/mode_warlords.c
    ${SRC_DIR}/apps/pong/modes/mode_power.c
)

set(TETRIS_APP_SOURCES
    ${SRC_DIR}/apps/tetris/tetris_app.c
    ${SRC_DIR}/apps/tetris/tetris_core.c
    ${SRC_DIR}/apps/tetris/tetris_rules.c
    ${SRC_DIR}/apps/tetris/tetris_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
)

set(FLAPPY_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/flappy")
set(FLAPPY_ASSET_C "${FLAPPY_ASSET_DIR}/flappy_assets.c")
set(FLAPPY_ASSET_H "${FLAPPY_ASSET_DIR}/flappy_assets.h")
set(FLAPPY_SOURCE_CACHE "${FLAPPY_ASSET_DIR}/upstream.zip")
add_custom_command(
    OUTPUT ${FLAPPY_ASSET_C} ${FLAPPY_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_flappy_assets.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${FLAPPY_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_flappy_assets.py
        --cache ${FLAPPY_SOURCE_CACHE}
        --output-c ${FLAPPY_ASSET_C}
        --output-h ${FLAPPY_ASSET_H}
    VERBATIM
)
add_custom_target(flappy_assets DEPENDS ${FLAPPY_ASSET_C} ${FLAPPY_ASSET_H})

set(FLAPPY_APP_SOURCES
    ${SRC_DIR}/apps/flappy/flappy_app.c
    ${SRC_DIR}/apps/flappy/flappy_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${FLAPPY_ASSET_C}
)

set(ARKANOID_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/arkanoid")
set(ARKANOID_ASSET_C "${ARKANOID_ASSET_DIR}/arkanoid_assets.c")
set(ARKANOID_ASSET_H "${ARKANOID_ASSET_DIR}/arkanoid_assets.h")
set(ARKANOID_SOURCE_CACHE "${ARKANOID_ASSET_DIR}/upstream.zip")
add_custom_command(
    OUTPUT ${ARKANOID_ASSET_C} ${ARKANOID_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arkanoid_assets.py
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ARKANOID_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_arkanoid_assets.py
        --cache ${ARKANOID_SOURCE_CACHE}
        --output-c ${ARKANOID_ASSET_C}
        --output-h ${ARKANOID_ASSET_H}
    VERBATIM
)
add_custom_target(arkanoid_assets DEPENDS ${ARKANOID_ASSET_C} ${ARKANOID_ASSET_H})

set(ARKANOID_APP_SOURCES
    ${SRC_DIR}/apps/arkanoid/arkanoid_app.c
    ${SRC_DIR}/apps/arkanoid/arkanoid_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${ARKANOID_ASSET_C}
)

set(TREX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/t-rex-runner")
set(TREX_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/trex")
set(TREX_ASSET_C "${TREX_ASSET_DIR}/trex_assets.c")
set(TREX_ASSET_H "${TREX_ASSET_DIR}/trex_assets.h")
set(TREX_SPRITE "${TREX_SOURCE_DIR}/assets/default_100_percent/100-offline-sprite.png")
add_custom_command(
    OUTPUT ${TREX_ASSET_C} ${TREX_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_trex_assets.py
        ${TREX_SPRITE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TREX_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_trex_assets.py
        --input ${TREX_SPRITE}
        --output-c ${TREX_ASSET_C}
        --output-h ${TREX_ASSET_H}
    VERBATIM
)
add_custom_target(trex_assets DEPENDS ${TREX_ASSET_C} ${TREX_ASSET_H})

set(TREX_APP_SOURCES
    ${SRC_DIR}/apps/trex/trex_app.c
    ${SRC_DIR}/apps/trex/trex_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${TREX_ASSET_C}
)

set(TWORLD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tworld")
set(TWORLD_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/tworld")
set(TWORLD_ASSET_C "${TWORLD_ASSET_DIR}/tworld_assets.c")
set(TWORLD_ASSET_H "${TWORLD_ASSET_DIR}/tworld_assets.h")
set(TWORLD_TILE_FILES "${TWORLD_SOURCE_DIR}/res/tiles.bmp")
add_custom_command(
    OUTPUT ${TWORLD_ASSET_C} ${TWORLD_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_tworld_assets.py
        ${TWORLD_TILE_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TWORLD_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_tworld_assets.py
        --source-root ${TWORLD_SOURCE_DIR}
        --output-c ${TWORLD_ASSET_C}
        --output-h ${TWORLD_ASSET_H}
    VERBATIM
)
add_custom_target(tworld_assets DEPENDS ${TWORLD_ASSET_C} ${TWORLD_ASSET_H})

set(TWORLD_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/chips-tworld.pak")
set(CHIPS_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/chips.pak")
add_custom_command(
    OUTPUT ${CHIPS_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        ${TWORLD_TILE_FILES}
        ${TWORLD_SOURCE_DIR}/intro.dat
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        --source-root ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/apps
        --only tworld
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TWORLD_PAK} ${CHIPS_PAK}
    VERBATIM
)
add_custom_target(dcapp_chips_assets ALL DEPENDS ${CHIPS_PAK})

set(OPENJAZZ_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/openjazz.pak")
add_custom_command(
    OUTPUT ${OPENJAZZ_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_openjazz_pack.py
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openjazz-shareware/JAZZ.ZIP
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_openjazz_pack.py
        --input ${CMAKE_CURRENT_SOURCE_DIR}/third_party/openjazz-shareware/JAZZ.ZIP
        --output ${OPENJAZZ_PAK}
    VERBATIM
)
add_custom_target(dcapp_openjazz_assets ALL DEPENDS ${OPENJAZZ_PAK})

set(TWORLD_CORE_SOURCES
    ${TWORLD_SOURCE_DIR}/encoding.c
    ${TWORLD_SOURCE_DIR}/mslogic.c
    ${TWORLD_SOURCE_DIR}/play.c
    ${TWORLD_SOURCE_DIR}/random.c
)

set(CHIPS_APP_SOURCES
    ${SRC_DIR}/apps/chips/chips_app.c
    ${SRC_DIR}/apps/chips/chips_port.c
    ${SRC_DIR}/apps/chips/chips_tworld_glue.c
    ${SRC_DIR}/apps/chips/chips_tworld_solution.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${TWORLD_CORE_SOURCES}
    ${TWORLD_ASSET_C}
)

set(PIPEDREAMER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/PipeDreamer")
set(PIPEDREAMER_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/pipe-pipedreamer.pak")
add_custom_command(
    OUTPUT ${PIPEDREAMER_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        ${PIPEDREAMER_SOURCE_DIR}/Source/Board.cpp
        ${PIPEDREAMER_SOURCE_DIR}/Source/Queue.cpp
        ${PIPEDREAMER_SOURCE_DIR}/Source/TilePiece.cpp
        ${PIPEDREAMER_SOURCE_DIR}/Resources/Images/PipeDreamerCanvas.png
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        --source-root ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/apps
        --only pipedreamer
    VERBATIM
)
add_custom_target(dcapp_pipe_assets ALL DEPENDS ${PIPEDREAMER_PAK})

set(PIPEDREAMER_CORE_SOURCES
    ${PIPEDREAMER_SOURCE_DIR}/Source/Board.cpp
    ${PIPEDREAMER_SOURCE_DIR}/Source/Queue.cpp
    ${PIPEDREAMER_SOURCE_DIR}/Source/Randomizer.cpp
    ${PIPEDREAMER_SOURCE_DIR}/Source/TilePiece.cpp
)

set(PIPE_APP_SOURCES
    ${SRC_DIR}/apps/pipe/pipe_app.cpp
    ${SRC_DIR}/apps/port/port_runtime.c
    ${PIPEDREAMER_CORE_SOURCES}
)

set(XSCORCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xscorch")
set(XSCORCH_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/xscorch")
set(XSCORCH_ASSET_C "${XSCORCH_ASSET_DIR}/xscorch_assets.c")
set(XSCORCH_ASSET_H "${XSCORCH_ASSET_DIR}/xscorch_assets.h")
add_custom_command(
    OUTPUT ${XSCORCH_ASSET_C} ${XSCORCH_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_xscorch_assets.py
        ${XSCORCH_SOURCE_DIR}/data/weapons.def
    COMMAND ${CMAKE_COMMAND} -E make_directory ${XSCORCH_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_xscorch_assets.py
        --source-root ${XSCORCH_SOURCE_DIR}
        --output-c ${XSCORCH_ASSET_C}
        --output-h ${XSCORCH_ASSET_H}
    VERBATIM
)
add_custom_target(xscorch_assets DEPENDS ${XSCORCH_ASSET_C} ${XSCORCH_ASSET_H})

set(XSCORCH_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/scorch-xscorch.pak")
add_custom_command(
    OUTPUT ${XSCORCH_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        ${XSCORCH_SOURCE_DIR}/data/weapons.def
        ${XSCORCH_SOURCE_DIR}/data/accessories.def
        ${XSCORCH_SOURCE_DIR}/img/xscorch-icon.xpm
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        --source-root ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/apps
        --only xscorch
    VERBATIM
)
add_custom_target(dcapp_scorch_assets ALL DEPENDS ${XSCORCH_PAK})

set(SCORCH_APP_SOURCES
    ${SRC_DIR}/apps/scorch/scorch_app.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${XSCORCH_ASSET_C}
)

set(XSOKOBAN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xsokoban")
set(XSOKOBAN_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/xsokoban")
set(XSOKOBAN_ASSET_C "${XSOKOBAN_ASSET_DIR}/xsokoban_assets.c")
set(XSOKOBAN_ASSET_H "${XSOKOBAN_ASSET_DIR}/xsokoban_assets.h")
file(GLOB XSOKOBAN_SCREEN_FILES CONFIGURE_DEPENDS "${XSOKOBAN_SOURCE_DIR}/screens/screen.*")
file(GLOB XSOKOBAN_TILE_FILES CONFIGURE_DEPENDS "${XSOKOBAN_SOURCE_DIR}/bitmaps/20/*.xpm")
add_custom_command(
    OUTPUT ${XSOKOBAN_ASSET_C} ${XSOKOBAN_ASSET_H}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_xsokoban_assets.py
        ${XSOKOBAN_SCREEN_FILES}
        ${XSOKOBAN_TILE_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${XSOKOBAN_ASSET_DIR}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_xsokoban_assets.py
        --source-root ${XSOKOBAN_SOURCE_DIR}
        --output-c ${XSOKOBAN_ASSET_C}
        --output-h ${XSOKOBAN_ASSET_H}
    VERBATIM
)
add_custom_target(xsokoban_assets DEPENDS ${XSOKOBAN_ASSET_C} ${XSOKOBAN_ASSET_H})

set(XSOKOBAN_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/sokoban-xsokoban.pak")
add_custom_command(
    OUTPUT ${XSOKOBAN_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        ${XSOKOBAN_SCREEN_FILES}
        ${XSOKOBAN_TILE_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_port_assets.py
        --source-root ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        --output-dir ${CMAKE_CURRENT_BINARY_DIR}/apps
        --only xsokoban
    VERBATIM
)
add_custom_target(dcapp_sokoban_assets ALL DEPENDS ${XSOKOBAN_PAK})

set(SOKOBAN_APP_SOURCES
    ${SRC_DIR}/apps/sokoban/sokoban_app.c
    ${SRC_DIR}/apps/sokoban/sokoban_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
    ${XSOKOBAN_ASSET_C}
)

set(OPENJAZZ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openjazz")
file(GLOB_RECURSE OPENJAZZ_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${OPENJAZZ_SOURCE_DIR}/src/*.cpp"
)
list(FILTER OPENJAZZ_ENGINE_SOURCES EXCLUDE REGEX "[/\\\\]src[/\\\\]io[/\\\\]sound\\.cpp$")
list(FILTER OPENJAZZ_ENGINE_SOURCES EXCLUDE REGEX "[/\\\\]src[/\\\\]jj2[/\\\\]")
list(FILTER OPENJAZZ_ENGINE_SOURCES EXCLUDE REGEX "[/\\\\]src[/\\\\]platforms[/\\\\]")
list(APPEND OPENJAZZ_ENGINE_SOURCES
    ${OPENJAZZ_SOURCE_DIR}/src/platforms/platform_interface.cpp
)

set(OPENJAZZ_APP_SOURCES
    ${SRC_DIR}/apps/openjazz/openjazz_app.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_cache.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_install.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_heap.c
    ${SRC_DIR}/apps/openjazz/openjazz_memory.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_pack.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_sdl.cpp
    ${SRC_DIR}/apps/openjazz/openjazz_sound_stub.cpp
    ${SRC_DIR}/apps/port/port_runtime.c
    ${OPENJAZZ_ENGINE_SOURCES}
    ${OPENJAZZ_SOURCE_DIR}/ext/argparse/argparse.c
    ${OPENJAZZ_SOURCE_DIR}/ext/miniz/miniz.c
    ${OPENJAZZ_SOURCE_DIR}/ext/stb/stb_rect_pack.c
)

set(SOCCER_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/soccer")
set(SOCCER_PAK "${CMAKE_CURRENT_BINARY_DIR}/apps/soccer-ysoccer.pak")
set(SOCCER_UPSTREAM_ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ysoccer19/upstream/java/android/assets")
file(GLOB_RECURSE SOCCER_UPSTREAM_ASSET_FILES CONFIGURE_DEPENDS
    "${SOCCER_UPSTREAM_ASSETS}/*"
)
add_custom_command(
    OUTPUT ${SOCCER_PAK}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_soccer_assets.py
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ysoccer19/DC32-README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ysoccer19/COPYING
        ${SOCCER_UPSTREAM_ASSET_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SOCCER_ASSET_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/apps
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/build_soccer_assets.py
        --source ${SOCCER_UPSTREAM_ASSETS}
        --output ${SOCCER_PAK}
    VERBATIM
)
add_custom_target(dcapp_soccer_assets ALL DEPENDS ${SOCCER_PAK})

set(SOCCER_APP_SOURCES
    ${SRC_DIR}/apps/soccer/soccer_app.c
    ${SRC_DIR}/apps/soccer/soccer_core.c
    ${SRC_DIR}/apps/soccer/soccer_port.c
    ${SRC_DIR}/apps/port/port_runtime.c
)

set_source_files_properties(
    ${OPENJAZZ_SOURCE_DIR}/src/main.cpp
    PROPERTIES COMPILE_DEFINITIONS "main=openjazz_main"
)

set_source_files_properties(
    ${OPENJAZZ_ENGINE_SOURCES}
    ${OPENJAZZ_SOURCE_DIR}/ext/argparse/argparse.c
    ${OPENJAZZ_SOURCE_DIR}/ext/miniz/miniz.c
    ${OPENJAZZ_SOURCE_DIR}/ext/stb/stb_rect_pack.c
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
)

set(RP2040_DOOM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/rp2040-doom")
set(DOOM_SRC_DIR "${RP2040_DOOM_DIR}/src")
set(DOOM_RENDER_DIR "${CMAKE_CURRENT_BINARY_DIR}/doom_dc32")
set(DOOM_RENDER_SOURCE "${DOOM_RENDER_DIR}/pd_render_dc32.cpp")
set(DOOM_LOOP_SOURCE "${DOOM_RENDER_DIR}/d_loop_dc32.c")
set(DOOM_D_MAIN_SOURCE "${DOOM_RENDER_DIR}/d_main_dc32.c")
set(DOOM_MENU_SOURCE "${DOOM_RENDER_DIR}/m_menu_dc32.c")
set(DOOM_R_DATA_WHD_SOURCE "${DOOM_RENDER_DIR}/r_data_whd_dc32.c")
set(DOOM_R_THINGS_SOURCE "${DOOM_RENDER_DIR}/r_things_dc32.c")
set(DOOM_VIDEO_SOURCE "${DOOM_RENDER_DIR}/v_video_dc32.c")

file(MAKE_DIRECTORY "${DOOM_RENDER_DIR}")
file(READ "${DOOM_SRC_DIR}/pd_render.cpp" DOOM_PD_RENDER_SOURCE)
string(REPLACE "#include \"picodoom.h\""
    "#include \"picodoom.h\"\n#include \"doom_dc32.h\""
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#define USE_CORE1_FOR_FLATS 1"
    "#define USE_CORE1_FOR_FLATS 0"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#define USE_CORE1_FOR_REGULAR 1"
    "#define USE_CORE1_FOR_REGULAR 0"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#define USE_INTERP PICO_ON_DEVICE"
    "#define USE_INTERP 0"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#if !PICO_RP2350\n#define RENDER_COL_MAX 3600\n#else\n#define RENDER_COL_MAX 7200\n#endif"
    "#define RENDER_COL_MAX DOOM_DC32_RENDER_COL_MAX"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "extern uint8_t __aligned(4) frame_buffer[2][SCREENWIDTH * MAIN_VIEWHEIGHT];"
    "extern uint8_t (*frame_buffer)[SCREENWIDTH * MAIN_VIEWHEIGHT];"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "static uint8_t __aligned(4) list_buffer[RENDER_COL_MAX * sizeof(pd_column) + 64*64]; // extra 64*64 is for one flat\nstatic uint8_t *last_list_buffer_limit = list_buffer + sizeof(list_buffer);\n//static_assert(text_font_cpy > list_buffer, \"\");\n#define MAX_CACHED_FLATS (sizeof(list_buffer) / 4096)"
    "extern uint8_t *doomDc32ListBuffer;\nextern uint32_t doomDc32ListBufferSize;\n#define list_buffer doomDc32ListBuffer\nstatic uint8_t *last_list_buffer_limit;\n#define MAX_CACHED_FLATS 8"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "void pd_init() {\n    sem_init(&core1_wake, 0, 1);"
    "void pd_init() {\n    last_list_buffer_limit = list_buffer + doomDc32ListBufferSize;\n    sem_init(&core1_wake, 0, 1);"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#if PICO_ON_DEVICE\n    static_assert(sizeof(vpatchlists_t) < 0xc00, \"\");\n#if !PICO_RP2350\n    vpatchlists = (vpatchlists_t *)(USBCTRL_DPRAM_BASE + 0x400);\n#else\n    static_assert(SRAM_SCRATCH_X_BASE - 0xc00 >= SHORTPTR_BASE + 0x4000, \"\"); // avoid potential heap locations\n    vpatchlists = (vpatchlists_t *)(SRAM_SCRATCH_X_BASE - 0xc00);\n#endif\n#else\n    vpatchlists = (vpatchlists_t*)malloc(sizeof(vpatchlists_t));\n#endif"
    "    vpatchlists = (vpatchlists_t*)doomDc32VpatchLists();"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "    uint8_t *list_buffer_limit = list_buffer + count_of(list_buffer);"
    "    uint8_t *list_buffer_limit = list_buffer + doomDc32ListBufferSize;"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "    sem_release(&core0_done);\n    sem_acquire_blocking(&core1_done);"
    "    if (USE_CORE1_FOR_FLATS || USE_CORE1_FOR_REGULAR) {\n        sem_release(&core0_done);\n        sem_acquire_blocking(&core1_done);\n    }"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "void pd_core1_loop() {\n#if PICO_ON_DEVICE"
    "void pd_core1_loop() {\n#if 0"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "#if PICO_ON_DEVICE\nextern \"C\" {\n#include \"i_picosound.h\"\n}"
    "#if 0\nextern \"C\" {\n#include \"i_picosound.h\"\n}"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "    sem_release(&render_frame_ready);\n    DEBUG_PINS_CLR(start_end, 2);"
    "    sem_release(&render_frame_ready);\n    doomDc32PresentFrame();\n    DEBUG_PINS_CLR(start_end, 2);"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "static uint16_t patch_decoder_tmp_table_patch_numbers[WHD_MAX_COL_UNIQUE_PATCHES];"
    "static uint16_t patch_decoder_tmp_table_patch_numbers[WHD_MAX_COL_UNIQUE_PATCHES];\nstatic void dc32_reset_patch_decoder_cache(void) {\n    memset(patch_hash_offsets, -1, sizeof(patch_hash_offsets));\n    memset(patch_decoder_circular_buf, 0, sizeof(patch_decoder_circular_buf));\n    patch_decoder_circular_buf_write_pos = 0;\n    patch_decoder_circular_buf_write_limit = PATCH_DECODER_CIRCULAR_BUFFER_SIZE;\n    memset(patch_decoder_tmp_table_patch_numbers, 0, sizeof(patch_decoder_tmp_table_patch_numbers));\n}"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "    memset(patch_hash_offsets, -1, sizeof(patch_hash_offsets));\n    patch_decoder_circular_buf_write_limit = PATCH_DECODER_CIRCULAR_BUFFER_SIZE;"
    "    dc32_reset_patch_decoder_cache();"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "        assert(slot == patch_hash(header->patch_num));\n        if (header->patch_num == patch_num) return offset;"
    "        if (slot != patch_hash(header->patch_num)) {\n            dc32_reset_patch_decoder_cache();\n            return ~slot;\n        }\n        if (header->patch_num == patch_num) return offset;"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "                        assert(*last != -1);\n                        *last = ((patch_hash_entry_header *) (patch_decoder_circular_buf + *last))->next;\n                        for (int p = 0; p < pdi_count; p++) {\n                            if (pdis[p].header.patch_num == header->patch_num) {\n                                pdis[p].decoder = nullptr;\n                            }\n                        }\n                        patch_decoder_circular_buf_write_limit += header->size;"
    "                        if (*last == -1 || header->size == 0 ||\n                            patch_decoder_circular_buf_write_limit + header->size > PATCH_DECODER_CIRCULAR_BUFFER_SIZE) {\n                            dc32_reset_patch_decoder_cache();\n                            for (int p = 0; p < pdi_count; p++) {\n                                pdis[p].decoder = nullptr;\n                            }\n                            offset_or_inverse_slot = patch_offset_or_inverse_slot(patch_num);\n                            continue;\n                        }\n                        *last = ((patch_hash_entry_header *) (patch_decoder_circular_buf + *last))->next;\n                        for (int p = 0; p < pdi_count; p++) {\n                            if (pdis[p].header.patch_num == header->patch_num) {\n                                pdis[p].decoder = nullptr;\n                            }\n                        }\n                        patch_decoder_circular_buf_write_limit += header->size;"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "        case GS_DEMOSCREEN:\n            patch_num = W_GetNumForName(pagename);\n            break;"
    "        case GS_DEMOSCREEN: {\n            int pnum = pagename ? W_CheckNumForName(pagename) : -1;\n            if (pnum >= 0) patch_num = pnum;\n            break;\n        }"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
string(REPLACE "                    int pnum = W_GetNumForName(pagename);\n                    assert(pnum);\n                    maybe_draw_single_screen(pnum);"
    "                    int pnum = pagename ? W_CheckNumForName(pagename) : -1;\n                    if (pnum >= 0) {\n                        maybe_draw_single_screen(pnum);\n                    } else {\n                        D_AdvanceDemo();\n                    }"
    DOOM_PD_RENDER_SOURCE "${DOOM_PD_RENDER_SOURCE}")
file(WRITE "${DOOM_RENDER_SOURCE}" "${DOOM_PD_RENDER_SOURCE}")

file(READ "${DOOM_SRC_DIR}/d_loop.c" DOOM_D_LOOP_SOURCE)
string(REPLACE "            if (net_client_connected && lplayer_count < 2) {\n                net_client_connected = false;\n                piconet_stop();\n            }"
    "#if USE_PICO_NET\n            if (net_client_connected && lplayer_count < 2) {\n                net_client_connected = false;\n                piconet_stop();\n            }\n#endif"
    DOOM_D_LOOP_SOURCE "${DOOM_D_LOOP_SOURCE}")
file(WRITE "${DOOM_LOOP_SOURCE}" "${DOOM_D_LOOP_SOURCE}")

file(READ "${DOOM_SRC_DIR}/doom/d_main.c" DOOM_D_MAIN_SOURCE_TEXT)
string(REPLACE "void D_AdvanceDemo (void)\n{\n    advancedemo = true;\n}"
    "void D_AdvanceDemo (void)\n{\n    advancedemo = true;\n}\n\nstatic void D_Dc32DeferDemoIfPresent(const char *name)\n{\n    if (name && W_CheckNumForName(name) >= 0)\n        G_DeferedPlayDemo(name);\n    else\n        D_AdvanceDemo();\n}"
    DOOM_D_MAIN_SOURCE_TEXT "${DOOM_D_MAIN_SOURCE_TEXT}")
string(REPLACE "G_DeferedPlayDemo(DEH_String(\"demo1\"));"
    "D_Dc32DeferDemoIfPresent(DEH_String(\"demo1\"));"
    DOOM_D_MAIN_SOURCE_TEXT "${DOOM_D_MAIN_SOURCE_TEXT}")
string(REPLACE "G_DeferedPlayDemo(DEH_String(\"demo2\"));"
    "D_Dc32DeferDemoIfPresent(DEH_String(\"demo2\"));"
    DOOM_D_MAIN_SOURCE_TEXT "${DOOM_D_MAIN_SOURCE_TEXT}")
string(REPLACE "G_DeferedPlayDemo(DEH_String(\"demo3\"));"
    "D_Dc32DeferDemoIfPresent(DEH_String(\"demo3\"));"
    DOOM_D_MAIN_SOURCE_TEXT "${DOOM_D_MAIN_SOURCE_TEXT}")
string(REPLACE "G_DeferedPlayDemo(DEH_String(\"demo4\"));"
    "D_Dc32DeferDemoIfPresent(DEH_String(\"demo4\"));"
    DOOM_D_MAIN_SOURCE_TEXT "${DOOM_D_MAIN_SOURCE_TEXT}")
file(WRITE "${DOOM_D_MAIN_SOURCE}" "${DOOM_D_MAIN_SOURCE_TEXT}")

file(READ "${DOOM_SRC_DIR}/doom/r_things.c" DOOM_R_THINGS_SOURCE_TEXT)
string(REPLACE "    if ((unsigned int) mobj_sprite(thing) >= (unsigned int) numsprites)\n        I_Error(\"R_ProjectSprite: invalid sprite number %i \",\n                mobj_sprite(thing));"
    "    if ((unsigned int) mobj_sprite(thing) >= (unsigned int) numsprites) {\n        if (demoplayback)\n            return;\n        I_Error(\"R_ProjectSprite: invalid sprite number %i \",\n                mobj_sprite(thing));\n    }"
    DOOM_R_THINGS_SOURCE_TEXT "${DOOM_R_THINGS_SOURCE_TEXT}")
string(REPLACE "    if ((mobj_frame(thing) & FF_FRAMEMASK) >= sprite_numframes(mobj_sprite(thing)))\n        I_Error(\"R_ProjectSprite: invalid sprite frame %i : %i \",\n                mobj_sprite(thing), mobj_frame(thing));"
    "    if ((mobj_frame(thing) & FF_FRAMEMASK) >= sprite_numframes(mobj_sprite(thing))) {\n        if (demoplayback)\n            return;\n        I_Error(\"R_ProjectSprite: invalid sprite frame %i : %i \",\n                mobj_sprite(thing), mobj_frame(thing));\n    }"
    DOOM_R_THINGS_SOURCE_TEXT "${DOOM_R_THINGS_SOURCE_TEXT}")
string(REPLACE "    if ((unsigned) psp->state->sprite >= (unsigned int) numsprites)\n        I_Error(\"R_ProjectSprite: invalid sprite number %i \",\n                psp->state->sprite);"
    "    if ((unsigned) psp->state->sprite >= (unsigned int) numsprites) {\n        if (demoplayback)\n            return;\n        I_Error(\"R_ProjectSprite: invalid sprite number %i \",\n                psp->state->sprite);\n    }"
    DOOM_R_THINGS_SOURCE_TEXT "${DOOM_R_THINGS_SOURCE_TEXT}")
string(REPLACE "    if ((psp->state->frame & FF_FRAMEMASK) >= sprite_numframes(psp->state->sprite))\n        I_Error(\"R_ProjectSprite: invalid sprite frame %i : %i \",\n                psp->state->sprite, psp->state->frame);"
    "    if ((psp->state->frame & FF_FRAMEMASK) >= sprite_numframes(psp->state->sprite)) {\n        if (demoplayback)\n            return;\n        I_Error(\"R_ProjectSprite: invalid sprite frame %i : %i \",\n                psp->state->sprite, psp->state->frame);\n    }"
    DOOM_R_THINGS_SOURCE_TEXT "${DOOM_R_THINGS_SOURCE_TEXT}")
file(WRITE "${DOOM_R_THINGS_SOURCE}" "${DOOM_R_THINGS_SOURCE_TEXT}")

file(READ "${DOOM_SRC_DIR}/doom/m_menu.c" DOOM_M_MENU_SOURCE)
string(REPLACE "#define BRIGHTNESS_MENU 1 // for badge"
    "#define BRIGHTNESS_MENU 0 // DC32 WHX omits M_BRIGHT"
    DOOM_M_MENU_SOURCE "${DOOM_M_MENU_SOURCE}")
string(REPLACE "    // \"Game\" for Network\n    V_DrawPatchDirect(OptionsDef.x + 105, OptionsDef.y + LINEHEIGHT * networkgame,\n                      VPATCH_HANDLE(VPATCH_NAME(M_GAME)));"
    "#if USE_PICO_NET\n    // \"Game\" for Network\n    V_DrawPatchDirect(OptionsDef.x + 105, OptionsDef.y + LINEHEIGHT * networkgame,\n                      VPATCH_HANDLE(VPATCH_NAME(M_GAME)));\n#endif"
    DOOM_M_MENU_SOURCE "${DOOM_M_MENU_SOURCE}")
file(WRITE "${DOOM_MENU_SOURCE}" "${DOOM_M_MENU_SOURCE}")

file(READ "${DOOM_SRC_DIR}/doom/r_data_whd.c" DOOM_R_DATA_WHD_SOURCE_TEXT)
string(REPLACE "#include <stdio.h>"
    "#include <stdio.h>\n#include <string.h>"
    DOOM_R_DATA_WHD_SOURCE_TEXT "${DOOM_R_DATA_WHD_SOURCE_TEXT}")
string(REPLACE "    whd_vpatch_numbers = (const uint16_t *) W_CacheLumpName(\"P_START\", PU_STATIC);\n    if (W_LumpLength(W_GetNumForName(\"P_START\")) != NUM_VPATCHES * 2) {\n        I_Error(\"whd is out of sync with VPATCH enum\"); // make sure whd_gen is up to date with code, and whd is regenerated\n    }"
    "    {\n        lumpindex_t pstart_lump = W_GetNumForName(\"P_START\");\n        const uint16_t *raw_vpatch_numbers = (const uint16_t *) W_CacheLumpNum(pstart_lump, PU_STATIC);\n        int pstart_len = W_LumpLength(pstart_lump);\n#if DC32_WHD_OMITS_M_BRIGHT_VPATCH\n        static uint16_t dc32_vpatch_numbers[NUM_VPATCHES];\n        if (pstart_len == (NUM_VPATCHES - 1) * 2) {\n            const int missing_vpatch = VPATCH_M_BRIGHT;\n            memcpy(dc32_vpatch_numbers, raw_vpatch_numbers, missing_vpatch * sizeof(dc32_vpatch_numbers[0]));\n            dc32_vpatch_numbers[missing_vpatch] = raw_vpatch_numbers[VPATCH_NAME_INVALID];\n            memcpy(dc32_vpatch_numbers + missing_vpatch + 1, raw_vpatch_numbers + missing_vpatch,\n                   pstart_len - missing_vpatch * sizeof(raw_vpatch_numbers[0]));\n            whd_vpatch_numbers = dc32_vpatch_numbers;\n        } else if (pstart_len == NUM_VPATCHES * 2) {\n            whd_vpatch_numbers = raw_vpatch_numbers;\n        } else {\n            I_Error(\"doom WHX vpatches %d expected %d/%d\", pstart_len / 2, NUM_VPATCHES - 1, NUM_VPATCHES);\n        }\n#else\n        whd_vpatch_numbers = raw_vpatch_numbers;\n        if (pstart_len != NUM_VPATCHES * 2) {\n            I_Error(\"whd is out of sync with VPATCH enum\"); // make sure whd_gen is up to date with code, and whd is regenerated\n        }\n#endif\n    }"
    DOOM_R_DATA_WHD_SOURCE_TEXT "${DOOM_R_DATA_WHD_SOURCE_TEXT}")
file(WRITE "${DOOM_R_DATA_WHD_SOURCE}" "${DOOM_R_DATA_WHD_SOURCE_TEXT}")

file(READ "${DOOM_SRC_DIR}/v_video.c" DOOM_VIDEO_SOURCE_TEXT)
string(REPLACE "        const patch_t *patch = resolve_vpatch_handle(patchlist[l].entry.patch_handle);"
    "        if (patchlist[l].entry.patch_handle == VPATCH_NAME_INVALID)\n            continue;\n        const patch_t *patch = resolve_vpatch_handle(patchlist[l].entry.patch_handle);"
    DOOM_VIDEO_SOURCE_TEXT "${DOOM_VIDEO_SOURCE_TEXT}")
string(REPLACE "        const uint8_t *pal;\n        if (!vpatch_has_shared_palette(patch)) {"
    "        if (vpatch_has_shared_palette(patch) && vpatch_shared_palette(patch) >= NUM_SHARED_PALETTES)\n            continue;\n        const uint8_t *pal;\n        if (!vpatch_has_shared_palette(patch)) {"
    DOOM_VIDEO_SOURCE_TEXT "${DOOM_VIDEO_SOURCE_TEXT}")
string(REPLACE "void V_DrawPatchN(int x, int y, vpatch_handle_large_t patch_handle, int repeat) {\n    int count;"
    "void V_DrawPatchN(int x, int y, vpatch_handle_large_t patch_handle, int repeat) {\n#if USE_WHD\n    if (patch_handle == VPATCH_NAME_INVALID)\n        return;\n#endif\n    int count;"
    DOOM_VIDEO_SOURCE_TEXT "${DOOM_VIDEO_SOURCE_TEXT}")
string(REPLACE "void V_DrawPatchFlipped(int x, int y, vpatch_handle_large_t patch_handle) {\n#if !USE_WHD"
    "void V_DrawPatchFlipped(int x, int y, vpatch_handle_large_t patch_handle) {\n#if USE_WHD\n    if (patch_handle == VPATCH_NAME_INVALID)\n        return;\n#endif\n#if !USE_WHD"
    DOOM_VIDEO_SOURCE_TEXT "${DOOM_VIDEO_SOURCE_TEXT}")
file(WRITE "${DOOM_VIDEO_SOURCE}" "${DOOM_VIDEO_SOURCE_TEXT}")

set(DOOM_COMMON_SOURCES
    ${DOOM_SRC_DIR}/m_argv.c
    ${DOOM_SRC_DIR}/m_misc.c
    ${DOOM_SRC_DIR}/i_sound.c
    ${DOOM_SRC_DIR}/d_event.c
    ${DOOM_SRC_DIR}/d_iwad.c
    ${DOOM_LOOP_SOURCE}
    ${DOOM_SRC_DIR}/d_mode.c
    ${DOOM_SRC_DIR}/deh_str.c
    ${DOOM_SRC_DIR}/gusconf.c
    ${DOOM_SRC_DIR}/m_bbox.c
    ${DOOM_SRC_DIR}/m_cheat.c
    ${DOOM_SRC_DIR}/m_config.c
    ${DOOM_SRC_DIR}/m_controls.c
    ${DOOM_SRC_DIR}/m_fixed.c
    ${DOOM_SRC_DIR}/net_client.c
    ${DOOM_SRC_DIR}/sha1.c
    ${DOOM_SRC_DIR}/memio.c
    ${DOOM_SRC_DIR}/tables.c
    ${DOOM_SRC_DIR}/v_diskicon.c
    ${DOOM_VIDEO_SOURCE}
    ${DOOM_SRC_DIR}/w_checksum.c
    ${DOOM_SRC_DIR}/w_main.c
    ${DOOM_SRC_DIR}/w_wad.c
    ${DOOM_SRC_DIR}/w_file.c
    ${SRC_DIR}/apps/doom/doom_w_file_memory.c
    ${DOOM_SRC_DIR}/w_merge.c
    ${DOOM_SRC_DIR}/z_zone.c
    ${DOOM_SRC_DIR}/tiny_huff.c
    ${DOOM_SRC_DIR}/musx_decoder.c
    ${DOOM_SRC_DIR}/image_decoder.c
)

set(DOOM_GAME_SOURCES
    ${DOOM_SRC_DIR}/doom/deh_ammo.c
    ${DOOM_SRC_DIR}/doom/deh_bexstr.c
    ${DOOM_SRC_DIR}/doom/deh_cheat.c
    ${DOOM_SRC_DIR}/doom/deh_doom.c
    ${DOOM_SRC_DIR}/doom/deh_frame.c
    ${DOOM_SRC_DIR}/doom/deh_misc.c
    ${DOOM_SRC_DIR}/doom/deh_ptr.c
    ${DOOM_SRC_DIR}/doom/deh_sound.c
    ${DOOM_SRC_DIR}/doom/deh_thing.c
    ${DOOM_SRC_DIR}/doom/deh_weapon.c
    ${DOOM_SRC_DIR}/doom/d_items.c
    ${DOOM_D_MAIN_SOURCE}
    ${DOOM_SRC_DIR}/doom/d_net.c
    ${DOOM_SRC_DIR}/doom/doomdef.c
    ${DOOM_SRC_DIR}/doom/doomstat.c
    ${DOOM_SRC_DIR}/doom/dstrings.c
    ${DOOM_SRC_DIR}/doom/f_wipe.c
    ${DOOM_SRC_DIR}/doom/g_game.c
    ${DOOM_SRC_DIR}/doom/hu_lib.c
    ${DOOM_SRC_DIR}/doom/hu_stuff.c
    ${DOOM_SRC_DIR}/doom/info.c
    ${DOOM_MENU_SOURCE}
    ${DOOM_SRC_DIR}/doom/m_random.c
    ${DOOM_SRC_DIR}/doom/p_ceilng.c
    ${DOOM_SRC_DIR}/doom/p_doors.c
    ${DOOM_SRC_DIR}/doom/p_enemy.c
    ${DOOM_SRC_DIR}/doom/p_floor.c
    ${DOOM_SRC_DIR}/doom/p_inter.c
    ${DOOM_SRC_DIR}/doom/p_lights.c
    ${DOOM_SRC_DIR}/doom/p_map.c
    ${DOOM_SRC_DIR}/doom/p_maputl.c
    ${DOOM_SRC_DIR}/doom/p_mobj.c
    ${DOOM_SRC_DIR}/doom/p_plats.c
    ${DOOM_SRC_DIR}/doom/p_pspr.c
    ${DOOM_SRC_DIR}/doom/p_setup.c
    ${DOOM_SRC_DIR}/doom/p_sight.c
    ${DOOM_SRC_DIR}/doom/p_spec.c
    ${DOOM_SRC_DIR}/doom/p_switch.c
    ${DOOM_SRC_DIR}/doom/p_telept.c
    ${DOOM_SRC_DIR}/doom/p_tick.c
    ${DOOM_SRC_DIR}/doom/p_user.c
    ${DOOM_SRC_DIR}/doom/r_bsp.c
    ${DOOM_R_DATA_WHD_SOURCE}
    ${DOOM_SRC_DIR}/doom/r_data.c
    ${DOOM_SRC_DIR}/doom/r_draw.c
    ${DOOM_SRC_DIR}/doom/r_main.c
    ${DOOM_SRC_DIR}/doom/r_plane.c
    ${DOOM_SRC_DIR}/doom/r_segs.c
    ${DOOM_SRC_DIR}/doom/r_sky.c
    ${DOOM_R_THINGS_SOURCE}
    ${DOOM_SRC_DIR}/doom/s_sound.c
    ${DOOM_SRC_DIR}/doom/sounds.c
    ${DOOM_SRC_DIR}/doom/statdump.c
    ${DOOM_SRC_DIR}/doom/st_lib.c
    ${DOOM_SRC_DIR}/doom/st_stuff.c
    ${DOOM_SRC_DIR}/doom/wi_stuff.c
)

set(DOOM_APP_SOURCES
    ${SRC_DIR}/apps/doom/doom_app.c
    ${SRC_DIR}/apps/doom/doom_dc32_platform.c
    ${SRC_DIR}/apps/doom/doom_dc32_sound.c
    ${SRC_DIR}/apps/doom/doom_dc32_stubs.c
    ${DOOM_RENDER_SOURCE}
    ${DOOM_COMMON_SOURCES}
    ${DOOM_GAME_SOURCES}
)

set_source_files_properties(
    ${SRC_DIR}/apps/doom/doom_app.c
    ${SRC_DIR}/apps/doom/doom_dc32_sound.c
    ${SRC_DIR}/apps/doom/doom_dc32_stubs.c
    PROPERTIES COMPILE_OPTIONS "-Os"
)

add_dcapp(dcapp_doom doom 206
	CATEGORY ports
    ${DOOM_APP_SOURCES}
)

target_include_directories(dcapp_doom BEFORE PRIVATE
    ${SRC_DIR}/apps/doom/compat
    ${SRC_DIR}/apps/doom
    ${DOOM_SRC_DIR}
    ${DOOM_SRC_DIR}/doom
    ${DOOM_RENDER_DIR}
)

target_compile_definitions(dcapp_doom PRIVATE
    PICO_BUILD=1
    PICO_ON_DEVICE=1
    PICO_RP2350=1
    DOOM_DC32_ENABLE_AUDIO=0
    DOOM_TINY=1
    SHRINK_MOBJ=1
    DOOM_ONLY=1
    DOOM_SMALL=1
    DOOM_CONST=1
    SOUND_LOW_PASS=1
    NUM_SOUND_CHANNELS=8
    NO_USE_CHECKSUM=1
    NO_USE_RELOAD=1
    USE_SINGLE_IWAD=1
    NO_USE_WIPE=1
    NO_USE_JOYSTICK=1
    NO_USE_MOUSE=1
    NO_USE_DEH=1
    NO_USE_MUSIC_PACKS=1
    USE_FLAT_MAX_256=1
    USE_MEMMAP_ONLY=1
    USE_LIGHTMAP_INDEXES=1
    USE_ERASE_FRAME=1
    NO_DRAW_MID=1
    NO_DRAW_TOP=1
    NO_DRAW_BOTTOM=1
    NO_DRAW_MASKED=1
    NO_DRAW_SKY=1
    NO_DRAW_SPRITES=1
    NO_DRAW_PSPRITES=1
    NO_VISPLANE_GUTS=1
    NO_VISPLANE_CACHES=1
    NO_DRAWSEGS=1
    NO_VISSPRITES=1
    NO_MASKED_FLOOR_CLIP=1
    PD_DRAW_COLUMNS=1
    PD_DRAW_MARKERS=1
    PD_DRAW_PLANES=1
    PD_SCALE_SORT=1
    PD_CLIP_WALLS=1
    PD_QUANTIZE=1
    PD_SANITY=1
    PD_COLUMNS=1
    PICO_DOOM=1
    NO_USE_DS_COLORMAP=1
    NO_USE_DC_COLORMAP=1
    USE_READONLY_MMAP=1
    USE_ROWAD=1
    NO_USE_TIMIDITY=1
    NO_USE_GUS=1
    NO_USE_LIBSAMPLERATE=1
    NO_USE_STATE_MISC=1
    USE_RAW_MAPNODE=1
    USE_RAW_MAPVERTEX=1
    USE_RAW_MAPSEG=1
    USE_RAW_MAPLINEDEF=1
    USE_RAW_MAPTHING=1
    USE_INDEX_LINEBUFFER=1
    NO_USE_ZLIGHT=1
    NO_Z_ZONE_ID=1
    Z_MALOOC_EXTRA_DATA=1
    USE_THINKER_POOL=1
    NO_INTERCEPTS_OVERRUN=1
    TEMP_IMMUTABLE_DISABLED=1
    USE_CONST_SFX=1
    USE_CONST_MUSIC=1
    NO_DEMO_RECORDING=1
    PICO_NO_TIMING_DEMO=1
    NO_USE_EXIT=1
    NO_RDRAW=1
    PICO_TIME_DEFAULT_ALARM_POOL_DISABLED=1
    PICO_TIME_DEFAULT_ALARM_POOL_MAX_TIMERS=1
    PICO_DISABLE_SHARED_IRQ_HANDLERS=1
    PICO_USE_OPTIMISTIC_SBRK=1
    NO_USE_NET=1
    USE_PICO_NET=0
    NO_FILE_ACCESS=1
    SAVE_COMPRESSED=1
    LOAD_COMPRESSED=1
    NO_USE_ARGS=1
    NO_USE_SAVE_CONFIG=1
    NO_USE_FLOAT=1
    USE_VANILLA_KEYBOARD_MAPPING_ONLY=1
    NO_USE_LOADING_DISK=1
    USE_WHD=1
    NO_Z_MALLOC_USER_PTR=1
    FIXED_SCREENWIDTH=1
    FLOOR_CEILING_CLIP_8BIT=1
    USE_MUSX=1
    MUSX_COMPRESSED=1
    NO_SCREENSHOT=1
    NO_USE_BOUND_CONFIG=1
    USE_FPS=1
    USE_MEMORY_WAD=1
    PICO_DEBUG_MALLOC=1
    EMU8950_NO_RATECONV=1
    EMU8950_NO_FLOAT=1
    PICO_CORE1_STACK_SIZE=0x4f8
    NO_IERROR=0
    PICODOOM_RENDER_NEWHOPE=1
    MERGE_DISTSCALE0_INTO_VIEWCOSSINANGLE=1
    PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=4
    PICO_SCANVIDEO_LINKED_SCANLINE_BUFFERS=1
    PICO_SCANVIDEO_SCANLINE_RELEASE_FUNCTION=1
    PICO_SCANVIDEO_MAX_SCANLINE_BUFFER_WORDS=164
    PICO_SCANVIDEO_PIXEL_BSHIFT=0
    PICO_SCANVIDEO_PIXEL_GSHIFT=6
    PICO_SCANVIDEO_PIXEL_RSHIFT=11
    WHD_SUPER_TINY=1
    DC32_WHD_OMITS_M_BRIGHT_VPATCH=1
    NO_USE_FINALE_CAST=1
    NO_USE_FINALE_BUNNY=1
    TINY_WAD_ADDR=0x10100000
    NO_USE_LOAD=1
    NO_USE_SAVE=1
    NO_USE_ENDDOOM=1
    USB_SUPPORT=0
    hard_assert=assert
)

target_compile_options(dcapp_doom PRIVATE
    # DOOM shares the fixed 256 KiB cache used by standard SD apps. Override
    # the firmware-wide -Ofast default so the release image has safe headroom.
    -Os
    -fno-common
    "-ffile-prefix-map=${CMAKE_SOURCE_DIR}=."
    "-ffile-prefix-map=${CMAKE_BINARY_DIR}=build"
    -Wno-error
    -Wno-format-truncation
    -Wno-array-bounds
    -Wno-cast-align
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -Wno-unused-const-variable
)

# Keep the frame renderer and per-tic simulation throughput-optimized. The
# remainder of the port is size-optimized above, where -Ofast's duplication
# does not buy visible frame rate.
set_source_files_properties(
    ${DOOM_RENDER_SOURCE}
    ${DOOM_LOOP_SOURCE}
    ${DOOM_R_THINGS_SOURCE}
    ${DOOM_VIDEO_SOURCE}
    ${DOOM_SRC_DIR}/doom/p_map.c
    ${DOOM_SRC_DIR}/doom/p_maputl.c
    ${DOOM_SRC_DIR}/doom/p_mobj.c
    ${DOOM_SRC_DIR}/doom/p_sight.c
    ${DOOM_SRC_DIR}/doom/p_tick.c
    ${DOOM_SRC_DIR}/doom/p_user.c
    ${DOOM_SRC_DIR}/doom/r_bsp.c
    ${DOOM_SRC_DIR}/doom/r_main.c
    ${DOOM_SRC_DIR}/doom/r_plane.c
    ${DOOM_SRC_DIR}/doom/r_segs.c
    PROPERTIES COMPILE_OPTIONS "-Ofast"
)

add_dcapp(dcapp_pong pong 200
	CATEGORY ports
    ${PONG_APP_SOURCES}
)

add_dcapp(dcapp_tetris tetris 201
	CATEGORY ports
    ${TETRIS_APP_SOURCES}
)

target_include_directories(dcapp_tetris BEFORE PRIVATE
    ${SRC_DIR}/apps/tetris
    ${SRC_DIR}/apps/port
)

add_dcapp(dcapp_arkanoid arkanoid 202
	CATEGORY ports
    ${ARKANOID_APP_SOURCES}
)

add_dependencies(dcapp_arkanoid arkanoid_assets)

target_include_directories(dcapp_arkanoid BEFORE PRIVATE
    ${ARKANOID_ASSET_DIR}
    ${SRC_DIR}/apps/arkanoid
    ${SRC_DIR}/apps/port
)

add_dcapp(dcapp_flappy flappy 203
	CATEGORY ports
    ${FLAPPY_APP_SOURCES}
)

add_dependencies(dcapp_flappy flappy_assets)

target_include_directories(dcapp_flappy BEFORE PRIVATE
    ${FLAPPY_ASSET_DIR}
    ${SRC_DIR}/apps/flappy
    ${SRC_DIR}/apps/port
)

add_dcapp(dcapp_trex trex 205
	CATEGORY ports
    ${TREX_APP_SOURCES}
)

add_dependencies(dcapp_trex trex_assets)

target_include_directories(dcapp_trex BEFORE PRIVATE
    ${TREX_ASSET_DIR}
    ${SRC_DIR}/apps/trex
    ${SRC_DIR}/apps/port
)

add_dcapp(dcapp_chips chips 207 LARGE_XIP
	CATEGORY ports
    ${CHIPS_APP_SOURCES}
)

add_dependencies(dcapp_chips tworld_assets)
add_dependencies(dcapp_chips_dcapp dcapp_chips_assets)

target_include_directories(dcapp_chips BEFORE PRIVATE
    ${TWORLD_ASSET_DIR}
    ${TWORLD_SOURCE_DIR}
    ${SRC_DIR}/apps/chips
    ${SRC_DIR}/apps/port
)

target_compile_definitions(dcapp_chips PRIVATE
    DC32_CHIPS_MS_ONLY=1
    malloc=dc32PortMalloc
    calloc=dc32PortCalloc
    realloc=dc32PortRealloc
    free=dc32PortFree
    time=chipsTworldTime
)

target_compile_options(dcapp_chips PRIVATE
    -UTICKS_PER_SECOND
    -Wno-error
    -Wno-format-truncation
    -Wno-implicit-fallthrough
    -Wno-old-style-definition
    -Wno-missing-prototypes
    -Wno-strict-prototypes
    -Wno-sign-compare
    -Wno-type-limits
    -Wno-unused-value
)

add_dcapp(dcapp_scorch scorch 208 LARGE_XIP
	CATEGORY ports
    ${SCORCH_APP_SOURCES}
)

add_dependencies(dcapp_scorch xscorch_assets)
add_dependencies(dcapp_scorch_dcapp dcapp_scorch_assets)

target_include_directories(dcapp_scorch BEFORE PRIVATE
    ${XSCORCH_ASSET_DIR}
    ${SRC_DIR}/apps/scorch
    ${SRC_DIR}/apps/port
)

target_compile_options(dcapp_scorch PRIVATE
    -Wno-error
    -Wno-format-truncation
)

add_dcapp(dcapp_pipe pipe 209 LARGE_XIP
	CATEGORY ports
    ${PIPE_APP_SOURCES}
)

add_dependencies(dcapp_pipe_dcapp dcapp_pipe_assets)

target_include_directories(dcapp_pipe BEFORE PRIVATE
    ${PIPEDREAMER_SOURCE_DIR}/Source
    ${SRC_DIR}/apps/pipe
    ${SRC_DIR}/apps/port
)

target_compile_options(dcapp_pipe PRIVATE
    -Wno-error
    $<$<COMPILE_LANGUAGE:CXX>:-frtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

add_dcapp(dcapp_sokoban sokoban 211
	CATEGORY ports
    ${SOKOBAN_APP_SOURCES}
)

add_dependencies(dcapp_sokoban xsokoban_assets)
add_dependencies(dcapp_sokoban_dcapp dcapp_sokoban_assets)

target_include_directories(dcapp_sokoban BEFORE PRIVATE
    ${XSOKOBAN_ASSET_DIR}
    ${SRC_DIR}/apps/sokoban
    ${SRC_DIR}/apps/port
)

add_dcapp(dcapp_openjazz openjazz 212 CXX_EXCEPTIONS
	CATEGORY ports
    ${OPENJAZZ_APP_SOURCES}
)
add_dependencies(dcapp_openjazz_dcapp dcapp_openjazz_assets)

target_include_directories(dcapp_openjazz BEFORE PRIVATE
    ${SRC_DIR}/apps/openjazz
    ${SRC_DIR}/apps/port
    ${OPENJAZZ_SOURCE_DIR}/src
    ${OPENJAZZ_SOURCE_DIR}/ext/argparse
    ${OPENJAZZ_SOURCE_DIR}/ext/miniz
    ${OPENJAZZ_SOURCE_DIR}/ext/stb
)

target_compile_definitions(dcapp_openjazz PRIVATE
    DC32_OPENJAZZ=1
    OJ_SDL1=1
    OJ_SDL2=0
    OJ_SDL3=0
    FULLSCREEN_ONLY=1
    NO_RESIZE=1
    DEFAULT_SCREEN_WIDTH=320
    DEFAULT_SCREEN_HEIGHT=240
    MUSIC_SETTINGS=0
    MINIZ_NO_ARCHIVE_WRITING_APIS=1
    MINIZ_NO_STDIO=1
    MINIZ_NO_TIME=1
    PORTABLE=1
    NDEBUG=1
)

target_compile_options(dcapp_openjazz PRIVATE
    -Wno-error
    -Wno-format-truncation
    -Wno-class-memaccess
    -Wno-unused-but-set-variable
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-sign-compare
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

add_dcapp(dcapp_soccer soccer 213 LARGE_XIP
	CATEGORY ports
    ${SOCCER_APP_SOURCES}
)

add_dependencies(dcapp_soccer_dcapp dcapp_soccer_assets)

target_include_directories(dcapp_soccer BEFORE PRIVATE
    ${SRC_DIR}/apps/soccer
    ${SRC_DIR}/apps/port
)

target_compile_options(dcapp_soccer PRIVATE
    -Wno-error
)

add_dcapp(dcapp_starfield starfield 220
	CATEGORY demos
    ${DEMOS_APP_SOURCES}
)

add_dcapp(dcapp_spiro spiro 221
	CATEGORY demos
    ${DEMOS_APP_SOURCES}
)

add_dcapp(dcapp_cube cube 222
	CATEGORY demos
    ${DEMOS_APP_SOURCES}
)

add_dcapp(dcapp_dvd_bounce dvd-bounce 223
	CATEGORY demos
    ${DEMOS_APP_SOURCES}
)

add_dcapp(dcapp_scroll_pattern scroll-pattern 224
	CATEGORY demos
    ${DEMOS_APP_SOURCES}
)
