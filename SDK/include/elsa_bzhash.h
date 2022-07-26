/***
 * @Author: insbread
 * @Date: 2022-07-21 15:19:25
 * @LastEditTime: 2022-07-21 15:19:25
 * @LastEditors: insbread
 * @Description: 基于网上流传的暴雪哈希算法实现的字符串哈希值计算函数，注意时哈希计算函数而不是哈希表
 * @FilePath: /elsa-server/SDK/include/elsa_bzhash.h
 * @版权声明
 */

#pragma once
/*** 基于暴雪哈希算法计算字符串的哈希值
 * @param {char} *str 需要计算的哈希字符串，C风格
 * @param {unsigned int} seed 哈希计算的随机种子数，相同种子数的哈希结果相同，否则不同
 * @return {usingned int} 哈希结果
 * @use:
 */
unsigned int bzhashstr(const char *str, unsigned int seed);