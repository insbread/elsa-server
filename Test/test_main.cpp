/*
 * @Author: insbread
 * @Date: 2022-07-14 17:10:17
 * @LastEditTime: 2022-07-14 17:10:18
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/Test/test_main.cpp
 * @版权声明
 */
// #define TEST_DEBUG
#ifdef TEST_DEBUG
// #include <iostream>
// #include <cstring>
// using namespace std;
// extern "C"
// {
// #include "lua.h"
// #include "lauxlib.h"
// #include "lualib.h"
// }

#include "elsac_second_time.h"
#include "memory/elsac_memory_counter.hpp"
#include "memory/elsac_base_allocator.hpp"
#include "memory/elsac_memory_container.hpp"
#include "memory/elsac_align_allocator.h"
#include "memory/elsac_buffer_allocator.h"
#include "memory/elsac_mempool.hpp"
#include "container/elsac_vector.h"
#include "container/elsac_linked_list.h"
#include "container/elsac_queue_list.h"
#include "container/elsac_obj_list.h"
#include "container/elsac_static_array_list.h"
#include "container/elsac_binary_list.h"
#include "container/elsac_static_hash_table.h"
#include "container/elsac_str_hash_table.h"
#include "datapacker/elsac_data_packet_reader.hpp"
#include "datapacker/elsac_data_packet.hpp"
#include "elsac_ref_obj.hpp"
#include "string/elsac_ansi_string.hpp"
#include "string/elsac_ref_string.hpp"
#include "elsac_stream.h"

int main()
{
    // ElsaCSecondTimeTestFunc();
    // ElsaCAllocatorCounterItemTest();
    // ElsaCMemoryCounterTest();
    // ElsaCBaseAllocatorTestFunc();
    // ElsaCMemoryContainerTestFunc();
    // ElsaCAlignAllocatorTestFunc();
    // ElsaCBufferAllocatorTestFunc();
    // ElsaCVectorTestFunc();
    // ElsaCMemPoolTest();
    // ElsaCBaseLinkedListTestFunc();
    // ElsaCListIteratorTestFunc();
    // ElsaCObjListTestFunc();
    // ElsaCStaticArrayListTestFuc();
    // ElsaCBinaryListTestFunc();
    // ElsaCStaticBinaryListTestFunc();
    // ElsaCStaticHashTableTestFunc();
    // ElsaCStrHashTableTestFunc();
    // ElsaCStrHashTableIteratorTestFunc();
    // ElsaCDataPacketReaderTestFunc1();
    // ElsaCDataPacketReaderTestFunc2();
    // ElsaCDataPacketTestFunc();
    // ElsaCRefObjectTestFunc();
    // ElsaCAnsiStringTestFunc();
    // ElsaCCTRefStringTestFunc();
    ElsaCFileStreamTestFunc();

    /* lua测试代码
    lua_State *L = luaL_newstate();

    lua_pushstring(L, "I am so cool~");
    lua_pushnumber(L, 20);

    if (lua_isstring(L, 1))
        cout << lua_tostring(L, 1) << endl;
    if (lua_isnumber(L, 2))
        cout << lua_tonumber(L, 2) << endl;

    lua_close(L);
    return;
    */
}

#endif