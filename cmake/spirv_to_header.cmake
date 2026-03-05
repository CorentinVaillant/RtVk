file(READ "${INPUT_FILE}" RAW_HEX HEX)
string(LENGTH "${RAW_HEX}" HEX_LEN)

math(EXPR REMAINDER "${HEX_LEN} % 8")
if(REMAINDER GREATER 0)
    math(EXPR PAD_COUNT "8 - ${REMAINDER}")
    string(REPEAT "0" ${PAD_COUNT} PADDING)
    string(APPEND RAW_HEX "${PADDING}")
endif()

set(LINES "")
set(LINE_ITEMS "")
set(COL 0)
set(POS 0)
while(POS LESS HEX_LEN)
    string(SUBSTRING "${RAW_HEX}" ${POS} 8 CHUNK)

    #little-endian → uint32_t 
    string(SUBSTRING "${CHUNK}" 0 2 B0)
    string(SUBSTRING "${CHUNK}" 2 2 B1)
    string(SUBSTRING "${CHUNK}" 4 2 B2)
    string(SUBSTRING "${CHUNK}" 6 2 B3)
    set(SWAPPED "${B3}${B2}${B1}${B0}")

    list(APPEND LINE_ITEMS "0x${SWAPPED}u")
    math(EXPR COL "${COL} + 1")
    math(EXPR POS "${POS} + 8")
    if(COL EQUAL 8)
        list(JOIN LINE_ITEMS ", " LINE_STR)
        list(APPEND LINES "    ${LINE_STR},")
        set(LINE_ITEMS "")
        set(COL 0)
    endif()
endwhile()


if(LINE_ITEMS)
    list(JOIN LINE_ITEMS ", " LINE_STR)
    list(APPEND LINES "    ${LINE_STR},")
endif()

list(JOIN LINES "\n" BODY)

file(WRITE "${OUTPUT_FILE}"
"#pragma once
#include <cstdint>

// Auto-generated from SPIR-V binary -- do not edit
constexpr uint32_t ${VARIABLE_NAME}[] = {
${BODY}
};
")

message(STATUS "Generated: ${OUTPUT_FILE}")