// 文件：types.h
// 作用：定宽整数类型定义，替代 libc 的 stdint.h，供所有内核头文件与源文件使用。
// 要点：uint8_t/uint16_t/uint32_t/uint64_t 分别对应 unsigned char/short/int/long long，无 size_t、ssize_t、bool 等定义；不包含任何标准库头文件。
#pragma once

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;
