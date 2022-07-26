cmake_minimum_required (VERSION 2.8.1)
project(Test)

# 设置到顶层的目录
set(TOP_DIR ${PROJECT_SOURCE_DIR}/..)   

include_directories(
    ${TOP_DIR}/SDK/include
    ${TOP_DIR}/Common/include/lua-5.1.5/src
)

link_directories(${TOP_DIR}/Common/libs)

set(CMAKE_C_FLAGS "-O0 -g3 -gdwarf-2 -std=c++0x -c -fmessage-length=0 -MMD -MP")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -Wall -Wextra -Wfatal-errors -std=c++11 -fpermissive -D_DEBUG -DTEST_DEBUG -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O0 -g3 -Wall -std=c++11 -fpermissive -g -ggdb -DNDEBUG -DTEST_DEBUG")

file(GLOB_RECURSE Test_CODELIST "${TOP_DIR}/*.cpp" "${TOP_DIR}/*.cc" "${TOP_DIR}/*.h" "${TOP_DIR}/*.hpp")

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/TestPath)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/TestPath)

# 添加链接库
set(TEST_LIBRARIES
    pthread
    lua
)

if ( (${CMAKE_BUILD_TYPE}) STREQUAL "Release" )
   set( _DEBUG  0 )
else ()
   set( _DEBUG  1 )
endif()

if ( _DEBUG )
  set( EXE_NAME test_d )
else()
  set( EXE_NAME test_r )
endif()

add_executable(${EXE_NAME} ${Test_CODELIST})
target_link_libraries(${EXE_NAME} ${TEST_LIBRARIES})