/***
 * @Author: insbread
 * @Date: 2022-07-21 15:20:14
 * @LastEditTime: 2022-07-21 15:20:15
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/elsa_bzhash.cpp
 * @版权声明
 */
#include "elsa_bzhash.h"
#define toupper(c) (((c) >= 'a' && (c) <= 'z') ? (c) ^ 0x20 : (c))

// 哈希运算种子表
static unsigned int cryptTable[0x500];
// 哈希种子表是否初始化
static int cryptTableInited = 0;

// 随机初始化运算种子表
static void initCryptrTable()
{
    unsigned int seed = 0x00100001, index1 = 0, index2 = 0, i = 0;

    for (index1 = 0; index1 < 0x100; index1++)
    {
        for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
        {
            unsigned int temp1, temp2;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);
            cryptTable[index2] = (temp1 | temp2);
        }
    }
}

unsigned int bzhashstr(const char *str, unsigned int seed)
{
    unsigned char *key = (unsigned char *)(str);
    unsigned int seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
    int ch;

    // 初始化随机种子表
    if (!cryptTableInited)
    {
        initCryptrTable();
        cryptTableInited = 1;
    }

    while (*key != 0)
    {
        ch = *key++;
        ch = toupper(ch);

        seed1 = cryptTable[(seed << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }
    return seed1;
}