# DC32 App SDK v1 CMake helper. Include this from an external app project.
set(DC32_APP_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "DC32 App SDK root")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(dc32_add_app TARGET OUTPUT_NAME RUNTIME_ID CATEGORY)
    add_executable(${TARGET} ${ARGN} "${DC32_APP_SDK_ROOT}/syscalls.c")
    target_include_directories(${TARGET} PRIVATE "${DC32_APP_SDK_ROOT}/include")
    target_compile_definitions(${TARGET} PRIVATE EMBEDDED DCAPP_BUILD=1 DCAPP_TOOL_BUILD=1
        DCAPP_RUNTIME_ID=${RUNTIME_ID} FATFS_USE_LFN_SUPPORT=1 USE_WIN_1252_CODE_PAGE)
    target_compile_options(${TARGET} PRIVATE -Os -Wall -Wextra -mthumb -mcpu=cortex-m33
        -mfloat-abi=soft -ffunction-sections -fdata-sections -fomit-frame-pointer)
    target_link_options(${TARGET} PRIVATE -nostartfiles -specs=nosys.specs -mthumb -mcpu=cortex-m33
        -mfloat-abi=soft -Wl,--gc-sections -Wl,--undefined=dcAppAbort
        -Wl,--undefined=dcAppRefreshDisplayOptions -Wl,-T${DC32_APP_SDK_ROOT}/linker_dcapp.lkr
        -Wl,-T${DC32_APP_SDK_ROOT}/dcapp_sdk_v1.ld)
    target_link_libraries(${TARGET} PRIVATE m)
    set(raw "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.raw")
    set(image "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.DC32")
    add_custom_command(OUTPUT ${image}
        DEPENDS ${TARGET}
        COMMAND ${CMAKE_OBJCOPY} -O binary -j .dcapp_header -j .text -j .data
            $<TARGET_FILE:${TARGET}> ${raw}
        COMMAND ${Python3_EXECUTABLE} ${DC32_APP_SDK_ROOT}/make_dcapp.py
            --nm ${CMAKE_NM} --elf $<TARGET_FILE:${TARGET}> --raw ${raw} --output ${image}
            --abi-version 1 --runtime-id ${RUNTIME_ID} --load-addr 0x10080000
            --app-ram-start 0x2005F000 --app-ram-size 0x14000
            --app-name ${OUTPUT_NAME} --category ${CATEGORY}
        VERBATIM)
    add_custom_target(${TARGET}_dcapp ALL DEPENDS ${image})
endfunction()
