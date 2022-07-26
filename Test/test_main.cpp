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
    ElsaCCTRefStringTestFunc();
}

#endif