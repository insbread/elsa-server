file(GLOB sources src/*.c)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../libs)

if(_DEBUG)
    add_library(lua-5.1_d ${LIBS_TYPE} ${sources})
else()
    add_library(lua-5.1_r ${LIBS_TYPE} ${sources})
endif()