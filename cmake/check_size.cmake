if(NOT DEFINED SIZE_TOOL OR NOT DEFINED ELF_FILE OR NOT DEFINED RAM_BUDGET)
    message(FATAL_ERROR "SIZE_TOOL, ELF_FILE, and RAM_BUDGET are required")
endif()

execute_process(
    COMMAND ${SIZE_TOOL} -A ${ELF_FILE}
    OUTPUT_VARIABLE SIZE_OUT
    RESULT_VARIABLE SIZE_RESULT
)
if(NOT SIZE_RESULT EQUAL 0)
    message(FATAL_ERROR "failed to run ${SIZE_TOOL} -A ${ELF_FILE}")
endif()

set(RAM_TOTAL 0)
foreach(section IN ITEMS .data .bss .ramvec)
    string(REGEX MATCH "${section}[ \t]+([0-9]+)" MATCHED "${SIZE_OUT}")
    if(MATCHED)
        math(EXPR RAM_TOTAL "${RAM_TOTAL} + ${CMAKE_MATCH_1}")
    endif()
endforeach()

if(RAM_TOTAL GREATER RAM_BUDGET)
    message(FATAL_ERROR "SRAM sections use ${RAM_TOTAL} bytes, over budget ${RAM_BUDGET}")
endif()

message(STATUS "SRAM budget ok: ${RAM_TOTAL}/${RAM_BUDGET} bytes")
